//
// Created by Weichuandong on 2025/3/25.
//

#include "SLAudioPlayer.h"

SLAudioPlayer::SLAudioPlayer(std::shared_ptr<SafeQueue<AVFrame *>> frameQueue,
                             std::shared_ptr<MediaSynchronizer> sync) :
    audioFrameQueue(std::move(frameQueue)),
    synchronizer(std::move(sync))
{
    // 分配音频缓冲区
    audioBuffer = new uint8_t[BUFFER_SIZE];
}

SLAudioPlayer::~SLAudioPlayer() {
    release();

    // 释放音频缓冲区
    delete audioBuffer;
    audioBuffer = nullptr;

    // 释放重采样资源
    if (swrContext) {
        swr_free(&swrContext);
        swrContext = nullptr;
    }

    if (resampleBuffer) {
        av_free(resampleBuffer);
        resampleBuffer = nullptr;
    }
}

bool SLAudioPlayer::prepare(int sampleRate, int channels, AVSampleFormat format) {
    std::lock_guard<std::mutex> lock(mutex);

    // 保存输入音频参数
    inSampleRate = sampleRate;
    inChannels = channels;
    inFormat = format;
    inChannelLayout = av_get_default_channel_layout(channels);

    LOGI("AudioPlayer: prepare play format : %dHz, %d通道, 格式%s, 通道布局为%ld",
         inSampleRate, inChannels, av_get_sample_fmt_name(inFormat), inChannelLayout);

    // 创建重采样上下文
    swrContext = swr_alloc_set_opts(nullptr,
                                    outChannelLayout,
                                    outFormat,
                                    outSampleRate,
                                    inChannelLayout,
                                    inFormat,
                                    inSampleRate,
                                    0, nullptr);
    if (!swrContext) {
        LOGE("AudioPlayer: can't create swrContext");
        return false;
    }

    int ret = swr_init(swrContext);
    if (ret < 0) {
        char errBuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errBuf, AV_ERROR_MAX_STRING_SIZE);
        LOGE("AudioPlayer: Failed to init swrContext: %s", errBuf);
        return false;
    }

    // 创建OpenSL ES引擎
    SLresult result = slCreateEngine(&engineObj, 0, nullptr, 0, nullptr, nullptr);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("AudioPlayer: Failed to init OpenSL ES Engine: %d", result);
        return false;
    }

    result = (*engineObj)->Realize(engineObj, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("AudioPlayer: Failed to Realize engineObj: %d", result);
        return false;
    }

    result = (*engineObj)->GetInterface(engineObj, SL_IID_ENGINE, &engine);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("AudioPlayer: Failed to GetInterface: %d", result);
        return false;
    }

    // 创建输出混音器
    result = (*engine)->CreateOutputMix(engine, &outputMixObj, 0, nullptr, nullptr);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("AudioPlayer: Failed to CreateOutputMix: %d", result);
        return false;
    }

    result = (*outputMixObj)->Realize(outputMixObj, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("AudioPlayer: Failed to Realize outputMixObj: %d", result);
        return false;
    }

    // 配置音频源
    SLDataLocator_AndroidSimpleBufferQueue locBufQ = {
            SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
            1
    };

    SLDataFormat_PCM formatPCM = {
            SL_DATAFORMAT_PCM,
            (SLuint32)outChannels,
            (SLuint32)(outSampleRate * 1000),  // mHz
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            outChannels == 1 ? SL_SPEAKER_FRONT_CENTER :
            (SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT),
            SL_BYTEORDER_LITTLEENDIAN
    };

    SLDataSource audioSrc = {&locBufQ, &formatPCM};

    // 配置音频接收器
    SLDataLocator_OutputMix locOutMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObj};
    SLDataSink audioSnk = {&locOutMix, nullptr};

    // 创建音频播放器, 提前进行接口预定
    const SLInterfaceID ids[] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME,
                                 SL_IID_PLAY};
    const SLboolean req[] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    result = (*engine)->CreateAudioPlayer(engine, &playerObj, &audioSrc, &audioSnk,
                                          3, ids, req);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("AudioPlayer: Failed to CreateAudioPlayer: %d", result);
        return false;
    }

    result = (*playerObj)->Realize(playerObj, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("AudioPlayer: Failed to Realize playerObj: %d", result);
        return false;
    }

    // 获取播放接口
    result = (*playerObj)->GetInterface(playerObj, SL_IID_PLAY, &player);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("AudioPlayer: Failed to GetInterface of player: %d", result);
        return false;
    }

    // 获取缓冲队列接口
    result = (*playerObj)->GetInterface(playerObj, SL_IID_BUFFERQUEUE, &bufferQueue);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("AudioPlayer: Failed to GetInterface of bufferQueue: %d", result);
        return false;
    }

    // 获取音量接口
    result = (*playerObj)->GetInterface(playerObj, SL_IID_VOLUME, &volumeItf);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("AudioPlayer: Failed to GetInterface of volumeItf: %d", result);
        // 继续，音量控制不是必须的
    }

    // 注册缓冲区回调
    result = (*bufferQueue)->RegisterCallback(bufferQueue, bufferQueueCallback, this);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("AudioPlayer: Failed to RegisterCallback of bufferQueue: %d", result);
        return false;
    }

    LOGI("AudioPlayer: Success to init OpenSL ES");
    isReady = true;
    return true;
}

void SLAudioPlayer::start() {
    if (!isReady) {
        LOGE("AudioPlayer: not ready, can't play");
        return;
    }

    // 清空音频缓冲区
    memset(audioBuffer, 0, BUFFER_SIZE);

    // 将第一个缓冲区加入队列
    SLresult result = (*bufferQueue)->Enqueue(bufferQueue, audioBuffer, BUFFER_SIZE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("AudioPlayer: can't enqueue audioBuffers: %d", result);
        return;
    }

    // 启动播放
    result = (*player)->SetPlayState(player, SL_PLAYSTATE_PLAYING);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("AudioPlayer: can't setPlayState: %d", result);
        return;
    }

    isRunning = true;
    LOGI("AudioPlayer: start play");
}

void SLAudioPlayer::pause() {
    if (player && isRunning) {
        (*player)->SetPlayState(player, SL_PLAYSTATE_PAUSED);
        isRunning = false;
        LOGI("AudioPlayer: pause play");
    }
}

void SLAudioPlayer::resume() {
    if (player && !isRunning) {
        (*player)->SetPlayState(player, SL_PLAYSTATE_PLAYING);
        isRunning = true;
        LOGI("AudioPlayer: resume play");
    }
}

void SLAudioPlayer::stop() {
    if (player) {
        (*player)->SetPlayState(player, SL_PLAYSTATE_STOPPED);
        isRunning = false;
        LOGI("AudioPlayer: stop play");
    }
}

void SLAudioPlayer::release() {
    stop();

    // 销毁OpenSL ES对象
    if (playerObj) {
        (*playerObj)->Destroy(playerObj);
        playerObj = nullptr;
        player = nullptr;
        bufferQueue = nullptr;
        volumeItf = nullptr;
    }

    if (outputMixObj) {
        (*outputMixObj)->Destroy(outputMixObj);
        outputMixObj = nullptr;
    }

    if (engineObj) {
        (*engineObj)->Destroy(engineObj);
        engineObj = nullptr;
        engine = nullptr;
    }

    isReady = false;
    LOGI("AudioPlayer: resource release");
}

void SLAudioPlayer::setVolume(float vol) {
    volume = std::max(0.0f, std::min(vol, 1.0f));

    if (volumeItf) {
        SLmillibel mb = vol > 0.01f ?
                        2000.0f * log10f(vol) :   // 转换到毫贝
                        SL_MILLIBEL_MIN;          // 静音

        (*volumeItf)->SetVolumeLevel(volumeItf, mb);
    }
}

void SLAudioPlayer::bufferQueueCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    auto* player = static_cast<SLAudioPlayer*>(context);
    player->processBuffer();
}

void SLAudioPlayer::processBuffer() {
    if (!isRunning) return;

    // 填充缓冲区
    fillBuffer(audioBuffer, BUFFER_SIZE);

    // 将缓冲区加入队列
    SLresult result = (*bufferQueue)->Enqueue(bufferQueue,
                                              audioBuffer,
                                              BUFFER_SIZE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("AudioPlayer: failed to enqueue audioBuffers: %d", result);
    }
}

void SLAudioPlayer::fillBuffer(uint8_t *buffer, int size) {
    if (!swrContext || !isRunning) {
        // 未初始化重采样器或未播放，填充静音
        memset(buffer, 0, size);
        return;
    }

    int bytesFilled = 0;

    // 如果之前重采样的数据可用
    if (availableSamples > 0 && resampleBuffer) {
        int bytesAvailable = availableSamples * outChannels * 2; // 16位立体声(通道数 * 2Byte)
        // 可以拷贝的字节数由缓冲区大小和可用数据量共同决定
        int bytesToCopy = std::min(size, bytesAvailable);

        memcpy(buffer, resampleBuffer, bytesToCopy);
        bytesFilled += bytesToCopy;

        // 如果数据还有剩余
        if (bytesToCopy < bytesAvailable) {
            // 部分使用，移动剩余数据
            memmove(resampleBuffer, resampleBuffer + bytesToCopy,
                    bytesAvailable - bytesToCopy);
            availableSamples -= bytesToCopy / (outChannels * 2);
        } else {
            // 全部使用
            availableSamples = 0;
        }
    }

    // 如果缓冲区没有填满
    while (bytesFilled < size) {
        AVFrame* frame = nullptr;
        if (!audioFrameQueue->pop(frame) || !frame) {
            // 没有帧可用，用静音填充
            memset(buffer + bytesFilled, 0, size - bytesFilled);
            break;
        }

        // 重采样音频帧
        int bytesResampled = resampleAudio(frame, buffer + bytesFilled, size - bytesFilled);
        bytesFilled += bytesResampled;

        // 更新音频时钟
        if (frame->pts != AV_NOPTS_VALUE) {
            double pts = frame->pts * av_q2d(AudioTimeBase);

            // 记录当前时间点
            audioClock.pts = pts;
            audioClock.lastUpdateTime = av_gettime() / 1000000.0;
            // 上传到主时钟
            synchronizer->update(audioClock, MediaSynchronizer::SyncSource::AUDIO);
        }

        av_frame_free(&frame);
    }

    // 应用音量
    applyVolume((int16_t*)buffer, size / 2);
}

int SLAudioPlayer::resampleAudio(AVFrame *frame, uint8_t *outBuffer, int outSize) {
    if (!swrContext || !frame || !frame->extended_data) return 0;

    // 计算输出样本数
    int outSamples = av_rescale_rnd(
            swr_get_delay(swrContext, frame->sample_rate) + frame->nb_samples,
            outSampleRate, frame->sample_rate, AV_ROUND_UP);

    // 计算输出数据大小
    int outBytes = outSamples * outChannels * 2; // 16位立体声

    // 当前帧获取的数据量超出outBuffer的接受量
    if (outBytes > outSize) {
        // 确保重采样缓冲区足够大
        if (outBytes > resampleBufferSize) {
            resampleBuffer = (uint8_t*)av_realloc(resampleBuffer, outBytes);
            resampleBufferSize = outBytes;
        }

        // 指向中间缓冲区
        uint8_t* outPtr = resampleBuffer;

        // 执行重采样
        int samplesConverted = swr_convert(
                swrContext,
                &outPtr, outSamples,
                (const uint8_t**)frame->extended_data, frame->nb_samples);

        if (samplesConverted <= 0) return 0;

        // 计算转换后的字节数
        int bytesConverted = samplesConverted * outChannels * 2;

        // 复制能放入输出缓冲区的部分
        int bytesToCopy = std::min(bytesConverted, outSize);
        memcpy(outBuffer, resampleBuffer, bytesToCopy);

        // 保存剩余的样本
        if (bytesToCopy < bytesConverted) {
            memmove(resampleBuffer, resampleBuffer + bytesToCopy,
                    bytesConverted - bytesToCopy);
            availableSamples = (bytesConverted - bytesToCopy) / (outChannels * 2);
        }

        return bytesToCopy;
    } else {
        // 直接重采样到输出缓冲区
        uint8_t* outPtr = outBuffer;

        int samplesConverted = swr_convert(
                swrContext,
                &outPtr, outSamples,
                (const uint8_t**)frame->extended_data, frame->nb_samples);

        if (samplesConverted <= 0) return 0;

        return samplesConverted * outChannels * 2;
    }
}

void SLAudioPlayer::applyVolume(int16_t *buffer, int numSamples) {
    float vol = volume.load();
    if (vol >= 0.999f) return; // 音量接近1.0，不需要处理

    for (int i = 0; i < numSamples; i++) {
        buffer[i] = (int16_t)(buffer[i] * vol);
    }
}

void SLAudioPlayer::setTimeBase(const AVRational &timeBase) {
    AudioTimeBase = timeBase;
}

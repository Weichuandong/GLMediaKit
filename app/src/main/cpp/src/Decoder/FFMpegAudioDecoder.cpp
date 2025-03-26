//
// Created by Weichuandong on 2025/3/25.
//
#include "Decoder/FFMpegAudioDecoder.h"

FFMpegAudioDecoder::FFMpegAudioDecoder(std::shared_ptr<SafeQueue<AVPacket *>> pktQueue,
                                       std::shared_ptr<SafeQueue<AVFrame *>> frameQueue) :
    audioPackedQueue(std::move(pktQueue)),
    audioFrameQueue(std::move(frameQueue))
{

}

FFMpegAudioDecoder::~FFMpegAudioDecoder() {

}

bool FFMpegAudioDecoder::configure(const AVCodecParameters *codecParams) {
    if (!codecParams) {
        LOGE("Invalid codec parameters");
        return false;
    }

    // 查找解码器
    const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
    if (!codec) {
        LOGE("Could not find decoder : %d", codecParams->codec_id);
        return false;
    }

    // 分配解码器上下文
    avCodecContext = avcodec_alloc_context3(codec);
    if (!avCodecContext) {
        LOGE("Could not alloc codecContext");
        return false;
    }
    // 将参数复制到解码器上下文
    if (avcodec_parameters_to_context(avCodecContext, codecParams) < 0) {
        LOGE("Could not parameters to codecContext");
        return false;
    }

    // 打开解码器
    if (avcodec_open2(avCodecContext, codec, nullptr) < 0) {
        LOGE("Could not open decoder");
        return -1;
    }

    inChannel = avCodecContext->channels;
    inSampleFormat = avCodecContext->sample_fmt;
    inSampleRate = avCodecContext->sample_rate;

    return true;
}

void FFMpegAudioDecoder::start() {
    if (isRunning()) return;
    LOGI("FFMpegAudioDecoder: start decode thread");
    exitRequested = false;
    isPaused = false;
    isReady = true;

    audioDecodeThread = std::thread(&FFMpegAudioDecoder::audioDecodeThreadFunc, this);
}

void FFMpegAudioDecoder::pause() {
    isPaused = true;
}

void FFMpegAudioDecoder::resume() {
    isPaused = false;
    pauseCond.notify_one();
}

void FFMpegAudioDecoder::stop() {
    exitRequested = true;
    resume();

    if (audioDecodeThread.joinable()) {
        audioDecodeThread.join();
    }
}

void FFMpegAudioDecoder::audioDecodeThreadFunc() {
    if (!avCodecContext || !audioPackedQueue || !audioFrameQueue) {
        LOGE("Audio decoder thread: invalid state");
        return;
    }

    AVPacket* packet = nullptr;
    auto lastLogTime = std::chrono::steady_clock::now();
    uint16_t frameCount = 0;

    while (!exitRequested) {

        // 检查是否暂停
        {
            std::unique_lock<std::mutex> lk(mtx);
            while (isPaused && !exitRequested) {
                pauseCond.wait(lk);
            }
        }

        if (exitRequested) break;

        // 从队列获取数据包
        if (!audioPackedQueue->pop(packet, 1)) {
            continue;
        }

        if (!packet) {
            // 空包，跳过
            continue;
        }


        // 发送包到解码器
        int sendResult = avcodec_send_packet(avCodecContext, packet);
        if (sendResult < 0) {
            if (sendResult != AVERROR(EAGAIN)) {
                LOGE("Error sending packet to decoder : %d", sendResult);
            }
            // 释放包
            av_packet_free(&packet);
            continue;
        }
        av_packet_free(&packet);

        // 接收解码后的帧
        AVFrame* frame = av_frame_alloc();
        int receiveResult;

        while ((receiveResult = avcodec_receive_frame(avCodecContext, frame)) >= 0) {
            // 复制帧以便安全地放入队列
            AVFrame* frameCopy = av_frame_alloc();
            if (!frameCopy) {
                av_frame_free(&frame);
                printf("Failed to allocate frame copy\n");
                break;
            }

            av_frame_ref(frameCopy, frame);

//            LOGI("Audio: frame->pts = %ld, frame->dts = %ld， time_base.num/time_base.den = %d/%d",
//                 frame->pts, frame->pkt_dts, avCodecContext->time_base.num, avCodecContext->time_base.den);
            // 放入帧队列
            if (!audioFrameQueue->push(frameCopy)) {
                // 队列满，丢弃帧
                av_frame_free(&frameCopy);
                LOGI("Frame queue full, dropping frame\n");
            }
            frameCount++;
            if (audioFrameQueue->getSize() > 10) {
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
            } else if (audioFrameQueue->getSize() > 25) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastLogTime);
            if (elapsed.count() >= 3000) {
                double elapsedSeconds = elapsed.count() / 1000.0;
                LOGI("音频解码统计: %d帧/%.3f秒 (%.2f帧/秒), 音频帧队列大小: %d, [精确毫秒:%lld]",
                     frameCount, elapsedSeconds,
                     frameCount/elapsedSeconds,
                     audioFrameQueue->getSize(), elapsed.count());
                frameCount = 0;
                lastLogTime = now;
            }
            // 重用同一个frame对象接收下一帧
            av_frame_unref(frame);
        }
    }
}


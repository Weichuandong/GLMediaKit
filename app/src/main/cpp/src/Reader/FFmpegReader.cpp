//
// Created by Weichuandong on 2025/3/28.
//

#include "Reader/FFmpegReader.h"

FFmpegReader::FFmpegReader(std::shared_ptr<SafeQueue<AVFrame *>> videoFrameQueue,
                                     std::shared_ptr<SafeQueue<AVFrame *>> audioFrameQueue,
                                     ReaderType type) :
        videoFrameQueue(std::move(videoFrameQueue)),
        audioFrameQueue(std::move(audioFrameQueue)),
        readerType(type),
        audioDemuxer(std::make_unique<FFmpegDemuxer>(FFmpegDemuxer::DemuxerType::AUDIO)),
        videoDemuxer(std::make_unique<FFmpegDemuxer>(FFmpegDemuxer::DemuxerType::VIDEO)),
        audioPacket(av_packet_alloc()),
        videoPacket(av_packet_alloc()),
        audioFrame(av_frame_alloc()),
        videoFrame(av_frame_alloc())
{

}

FFmpegReader::~FFmpegReader() {
    releaseAudio();
    releaseVideo();
}

bool FFmpegReader::open(const std::string &file_path) {
    filePath = file_path;

    // 打开音频
    if (!audioDemuxer->open(filePath)) {
        LOGE("failed to open file for audio: %s", file_path.c_str());
        return false;
    }

    if (hasAudio()) {
        audioDecoder = std::make_unique<FFmpegAudioDecoder>();
        auto config = DecoderConfig();
        config.param = audioDemuxer->getCodecParameters();
        if (!audioDecoder->configure(config)) {
            LOGE("failed to configure audioDecoder");
            return false;
        }
    } else {
        releaseAudio();
        audioDemuxer.reset();
    }

    // 打开视频
    if (!videoDemuxer->open(filePath)) {
        LOGE("failed to open file for video: %s", file_path.c_str());
        return false;
    }
    if (hasVideo()) {
//        videoDecoder = std::make_unique<FFmpegVideoDecoder>();
        videoDecoder = std::make_unique<MediaCodecVideoDecoder>();
        auto config = DecoderConfig();
        // 获取SPS，PPS
        std::vector<uint8_t> sps, pps;
//        videoDemuxer->getCodecParameters()
        extractSPSPPS(videoDemuxer->getCodecParameters(), sps, pps);
        config.type = "video/avc";
        config.format = config.fromAVFormat(static_cast<AVPixelFormat>(videoDemuxer->getCodecParameters()->format));
        config.width = videoDemuxer->getCodecParameters()->width;
        config.height = videoDemuxer->getCodecParameters()->height;
        config.param = videoDemuxer->getCodecParameters();
        config.extraData.emplace("sps", sps);
        config.extraData.emplace("pps", pps);
        if (!videoDecoder->configure(config)) {
            LOGE("failed to configure videoDecoder");
            return false;
        }

        OpenBsfCtx();

    } else {
        releaseVideo();
        videoDemuxer.reset();
    }

    return true;
}

void FFmpegReader::start() {
    if (isRunning()) return;
    exitRequested = false;
    isPaused = false;
    isReady = true;

    if (hasAudio()) audioReadThread = std::thread(&FFmpegReader::audioReadThreadFunc, this);
    if (hasVideo()) videoReadThread = std::thread(&FFmpegReader::videoReadThreadFunc, this);
}

void FFmpegReader::pause() {
    LOGI("FFmpegReader pause");
    isPaused = true;
    audioFrameQueue->pause();
    videoFrameQueue->pause();
}

void FFmpegReader::resume() {
    isPaused = false;
    if (hasAudio()) audioPauseCond.notify_all();
    if (hasVideo()) videoPauseCond.notify_all();
}

void FFmpegReader::stop() {
    exitRequested = true;
    resume();
    LOGI("before flush, videoFrameQueue->getSize() = %d, audioFrameQueue->getSize() = %d",
         videoFrameQueue->getSize(), audioFrameQueue->getSize());
    audioFrameQueue->flush();
    videoFrameQueue->flush();
    audioFrameQueue->resume();
    videoFrameQueue->resume();

    if (audioReadThread.joinable()) {
        audioReadThread.join();
    }
    if (videoReadThread.joinable()) {
        videoReadThread.join();
    }

    LOGI("after stop, videoFrameQueue->getSize() = %d, audioFrameQueue->getSize() = %d",
         videoFrameQueue->getSize(), audioFrameQueue->getSize());
}

void FFmpegReader::seekTo(double position) {

}

bool FFmpegReader::hasVideo() const {
    return videoDemuxer->hasVideo();
}

bool FFmpegReader::hasAudio() const {
    return audioDemuxer->hasAudio();
}

AVRational FFmpegReader::getAudioTimeBase() const {
    return audioDemuxer->getTimeBase();
}

AVRational FFmpegReader::getVideoTimeBase() const {
    return videoDemuxer->getTimeBase();
}

void FFmpegReader::audioReadThreadFunc() {
    if (!isReadying()) {
        LOGE("Reader is not ready, exit readThread");
        return;
    }
    LOGI("FFmpegReader : start audio read thread");

    auto lastLogTime = std::chrono::steady_clock::now();
    uint16_t audioPacketCount = 0;
    uint16_t audioFrameCount = 0;
    // 路径记得是安卓端的
    FILE *outfile = nullptr;
    if (downAudio) {
        outfile = fopen("/sdcard/output.pcm", "wb");
        if (!outfile) {
            LOGE("无法创建输出文件");
        }
    }

    while (!exitRequested) {
        {
            // 是否暂停
            std::unique_lock<std::mutex> lk(audioMtx);
            while (isPaused && !exitRequested) {
                audioPauseCond.wait(lk);
            }
        }

        if (exitRequested) break;

        // 获取packet
        audioDemuxer->ReceivePacket(audioPacket);
        audioPacketCount++;
        std::shared_ptr<IMediaPacket> mediaPacket = std::make_shared<FFmpegPacket>(audioPacket);
        std::shared_ptr<IMediaFrame> mediaFrame = std::make_shared<FFmpegFrame>(audioFrame);
        if (audioDecoder->SendPacket(mediaPacket) == 0) {
            LOGE("audioDecoder SendPacket failed");
        }
        while (audioDecoder->ReceiveFrame(mediaFrame) == 0) {
            AVFrame* cloneFrame = av_frame_clone(audioFrame);

            if (downAudio) {
                int bytes_per_sample = av_get_bytes_per_sample(
                        static_cast<AVSampleFormat>(audioFrame->format));
                int is_planar = av_sample_fmt_is_planar(static_cast<AVSampleFormat>(audioFrame->format));
                if (is_planar) {
                    for (int s = 0; s < audioFrame->nb_samples; ++s) {
                        for (int ch = 0; ch < audioFrame->channels; ++ch) {
                            fwrite(audioFrame->data[ch] + s * bytes_per_sample, 1, bytes_per_sample, outfile);
                        }
                    }
                }
            }

            if (!audioFrameQueue->push(cloneFrame)) {
                LOGE("Failed to push frame to audioFrameQueue");
                av_frame_unref(cloneFrame);
            }
            audioFrameCount++;
            av_frame_unref(audioFrame);
        }

        av_packet_unref(audioPacket);
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastLogTime).count();
        if (elapsed >= 3000) {
            LOGI("音频解封装统计: %d帧/%.3f秒 (%.2f帧/秒)",
                 audioPacketCount, elapsed / 1000.0,
                 audioPacketCount / (elapsed / 1000.0f));
            LOGI("音频解码统计: %d帧/%.3f秒 (%.2f帧/秒), 队列大小: %d",
                 audioFrameCount, elapsed / 1000.0,
                 audioFrameCount / (elapsed / 1000.0f), audioFrameQueue->getSize());
            lastLogTime = now;
            audioPacketCount = audioFrameCount = 0;
        }
    }
    if (downAudio) {
        fclose(outfile);
    }
}

void FFmpegReader::videoReadThreadFunc() {
    if (!isReadying()) {
        LOGE("Reader is not ready, exit readThread");
        return;
    }
    LOGI("FFmpegReader : start video read thread");

    auto lastLogTime = std::chrono::steady_clock::now();
    uint16_t videoPacketCount = 0;
    uint16_t videoFrameCount = 0;
    while (!exitRequested) {
        {
            std::unique_lock<std::mutex> lk(videoMtx);
            while (isPaused && !exitRequested) {
                videoPauseCond.wait(lk);
            }

            if (exitRequested) break;

            videoDemuxer->ReceivePacket(videoPacket);
            videoPacketCount++;
            std::shared_ptr<IMediaPacket> mediaPacket = std::make_shared<FFmpegPacket>(videoPacket);
            ConvertAVCCToAnnexB(mediaPacket->asAVPacket());
            std::shared_ptr<IMediaFrame> mediaFrame = std::make_shared<FFmpegFrame>(videoFrame);
            if (videoDecoder->SendPacket(mediaPacket) != 0) {
                LOGE("VideoDecoder SendPacket failed");
            }
            while (videoDecoder->ReceiveFrame(mediaFrame) == 0) {
                AVFrame* cloneFrame = av_frame_clone(videoFrame);
                if (!videoFrameQueue->push(cloneFrame)) {
                    LOGE("Failed to push frame to audioFrameQueue");
                    av_frame_unref(cloneFrame);
                }
                videoFrameCount++;
                av_frame_unref(videoFrame);
            }

            av_packet_unref(videoPacket);
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastLogTime).count();
            if (elapsed >= 3000) {
                LOGI("视频解封装统计: %d帧/%.3f秒 (%.2f帧/秒)",
                     videoPacketCount, elapsed / 1000.0,
                     videoPacketCount / (elapsed / 1000.0f));
                LOGI("视频解码统计: %d帧/%.3f秒 (%.2f帧/秒), 队列大小: %d",
                     videoFrameCount, elapsed / 1000.0,
                     videoFrameCount / (elapsed / 1000.0f), videoFrameQueue->getSize());
                lastLogTime = now;
                videoPacketCount = videoFrameCount = 0;
            }
        }
    }
}

void FFmpegReader::releaseAudio() {
    if (audioPacket) {
        av_packet_free(&audioPacket);
    }

    if (audioFrame) {
        av_frame_free(&audioFrame);
    }
}

void FFmpegReader::releaseVideo() {
    if (videoPacket) {
        av_packet_free(&videoPacket);
    }
    if (videoFrame) {
        av_frame_free(&videoFrame);
    }
}

bool FFmpegReader::extractSPSPPS(AVCodecParameters *codecParams, std::vector<uint8_t> &sps,
                                 std::vector<uint8_t> &pps) {
    if (!codecParams || !codecParams->extradata || codecParams->extradata_size < 7) {
        LOGE("Invalid extradata");
        return false;
    }
    AVFormatContext* fmt;

    const uint8_t* extradata = codecParams->extradata;
    int extradata_size = codecParams->extradata_size;
    std::vector<uint8_t> startCode = {0x00, 0x00, 0x00, 0x01};

    // 检查是否为AVCC格式（一般mp4容器采用）
    if (extradata[0] == 1) { // AVCC格式以1开头
        int offset = 5;      // 跳过版本、profile、compatibility、level和长度字段

        while (true) {
            // 获取SPS或者PPS个数,第六个字节[111......]前三个bit默认为111，后五个bit表示接下来的SPS或者PPS个数
            int num = extradata[offset] & 0x1F;
            offset++;

            for (int i = 0; i < num; ++i) {
                // 读取当前SPS或者PPS长度, 2字节大端序
                int length = (extradata[offset] << 8) | extradata[offset + 1];
                offset += 2;
                // 获取类型
                int type = extradata[offset] & 0x0F;

                // 目前只保留第一个SPS或PPS
                if (i == 0) {
                    if (type == 7) {
                        sps.assign(startCode.begin(), startCode.end());
                        sps.reserve(4 + length);
                        sps.insert(sps.end(), extradata + offset, extradata + offset + length);
                        offset += length;
                        break;
                    } else if (type == 8) {
                        pps.assign(startCode.begin(), startCode.end());
                        pps.reserve(4 + length);
                        pps.insert(pps.end(), extradata + offset, extradata + offset + length);
                        offset += length;
                        break;
                    }
                }
            }
            if (!sps.empty() && !pps.empty()) {
                break;
            }
        }
        return !sps.empty() && ! pps.empty();
    } else if (extradata[0] == 0 && extradata[1] == 0 &&
               extradata[2] == 0 && extradata[3] == 1){ // 格式为Annex-B(开头是起始码0x00000001)


    }

    return false;
}

int FFmpegReader::ConvertAVCCToAnnexB(AVPacket *packet) {
    char errString[128];
    int ret = av_bsf_send_packet(m_absCtx, packet);
    if (ret != 0) {
        av_strerror(ret, errString, 128);
        LOGE("convert packet from avcc to annexb failed when send packet! ret=%d, msg=%s", ret, errString);
        return ret;
    }

    ret = av_bsf_receive_packet(m_absCtx, packet);
    if (ret != 0) {
        av_strerror(ret, errString, 128);
        LOGE("convert packet from avcc to annexb failed when receive packet! ret=%d, msg=%s", ret, errString);
        return ret;
    }

    return 0;
}

int FFmpegReader::OpenBsfCtx() {
    const AVBitStreamFilter *bsFilter = nullptr;
    AVCodecParameters *codecParameters = nullptr;

    auto videoStream = videoDemuxer->getAVStream();

    auto codecID = videoStream->codecpar->codec_id;
    auto codecDesc = avcodec_descriptor_get(codecID);
    if (codecDesc == nullptr) {
        LOGE("Failed to get codec descriptor for '%s'!", avcodec_get_name(codecID));
        return -1;
    }

    // 1. 找到相应解码器的过滤器
    if (strcasecmp(codecDesc->name, "h264") == 0) {
        bsFilter = av_bsf_get_by_name("h264_mp4toannexb");
    } else if (strcasecmp(codecDesc->name, "h265") == 0 || strcasecmp(codecDesc->name, "hevc") == 0) {
        bsFilter = av_bsf_get_by_name("hevc_mp4toannexb");
    }

    if (bsFilter == nullptr) {
        LOGE("Can not get bsf by name");
        return -1;
    }

    // 2.过滤器分配内存
    if (av_bsf_alloc(bsFilter, &m_absCtx) != 0) {
        LOGE("av_bsf_alloc is failed");
        return -2;
    }

    // 3. 添加解码器属性
    if (videoStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        codecParameters = videoStream->codecpar;
    }

    if (avcodec_parameters_copy(m_absCtx->par_in, codecParameters) < 0) {
        LOGE("avcodec_parameters_copy is failed");
        return -1;
    }

    // 4. 初始化过滤器上下文
    if (av_bsf_init(m_absCtx) < 0) {
        LOGE("av_bsf_init is failed");
        return -1;
    }

    return 0;
}

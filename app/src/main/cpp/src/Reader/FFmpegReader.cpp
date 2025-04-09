//
// Created by Weichuandong on 2025/3/28.
//

#include "Reader/FFmpegReader.h"

FFMpegReader::FFMpegReader(std::shared_ptr<SafeQueue<AVFrame *>> videoFrameQueue,
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

FFMpegReader::~FFMpegReader() {
    releaseAudio();
    releaseVideo();
}

bool FFMpegReader::open(const std::string &file_path) {
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
        videoDecoder = std::make_unique<FFmpegVideoDecoder>();
//        videoDecoder = std::make_unique<MediaCodecVideoDecoder>();
        auto config = DecoderConfig();
        // 获取SPS，PPS
//        std::vector<uint8_t> sps, pps;
//        extractSPSPPS(videoDemuxer->getCodecParameters(), sps, pps);
        config.param = videoDemuxer->getCodecParameters();
//        config.extraData.emplace("sps", sps);
//        config.extraData.emplace("pps", pps);
        if (!videoDecoder->configure(config)) {
            LOGE("failed to configure videoDecoder");
            return false;
        }
    } else {
        releaseVideo();
        videoDemuxer.reset();
    }

    return true;
}

void FFMpegReader::start() {
    if (isRunning()) return;
    exitRequested = false;
    isPaused = false;
    isReady = true;

    if (hasAudio()) audioReadThread = std::thread(&FFMpegReader::audioReadThreadFunc, this);
    if (hasVideo()) videoReadThread = std::thread(&FFMpegReader::videoReadThreadFunc, this);
}

void FFMpegReader::pause() {
    LOGI("FFmpegReader pause");
    isPaused = true;
    audioFrameQueue->pause();
    videoFrameQueue->pause();
}

void FFMpegReader::resume() {
    isPaused = false;
    if (hasAudio()) audioPauseCond.notify_all();
    if (hasVideo()) videoPauseCond.notify_all();
}

void FFMpegReader::stop() {
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

void FFMpegReader::seekTo(double position) {

}

bool FFMpegReader::hasVideo() const {
    return videoDemuxer->hasVideo();
}

bool FFMpegReader::hasAudio() const {
    return audioDemuxer->hasAudio();
}

AVRational FFMpegReader::getAudioTimeBase() const {
    return audioDemuxer->getTimeBase();
}

AVRational FFMpegReader::getVideoTimeBase() const {
    return videoDemuxer->getTimeBase();
}

void FFMpegReader::audioReadThreadFunc() {
    if (!isReadying()) {
        LOGE("Reader is not ready, exit readThread");
        return;
    }
    LOGI("FFMpegReader : start audio read thread");

    auto lastLogTime = std::chrono::steady_clock::now();
    uint16_t audioPacketCount = 0;
    uint16_t audioFrameCount = 0;

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
            while (audioDecoder->ReceiveFrame(mediaFrame) == 0) {
                AVFrame* cloneFrame = av_frame_clone(audioFrame);
                if (!audioFrameQueue->push(cloneFrame)) {
                    LOGE("Failed to push frame to audioFrameQueue");
                    av_frame_unref(cloneFrame);
                }
                audioFrameCount++;
                av_frame_unref(audioFrame);
            }
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
}

void FFMpegReader::videoReadThreadFunc() {
    if (!isReadying()) {
        LOGE("Reader is not ready, exit readThread");
        return;
    }
    LOGI("FFMpegReader : start video read thread");

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
            std::shared_ptr<IMediaFrame> mediaFrame = std::make_shared<FFmpegFrame>(videoFrame);
            if (videoDecoder->SendPacket(mediaPacket) == 0) {
                while (videoDecoder->ReceiveFrame(mediaFrame) == 0) {
                    AVFrame* cloneFrame = av_frame_clone(videoFrame);
                    if (!videoFrameQueue->push(cloneFrame)) {
                        LOGE("Failed to push frame to audioFrameQueue");
                        av_frame_unref(cloneFrame);
                    }
                    videoFrameCount++;
                    av_frame_unref(videoFrame);
                }
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

void FFMpegReader::releaseAudio() {
    if (audioPacket) {
        av_packet_free(&audioPacket);
    }

    if (audioFrame) {
        av_frame_free(&audioFrame);
    }
}

void FFMpegReader::releaseVideo() {
    if (videoPacket) {
        av_packet_free(&videoPacket);
    }
    if (videoFrame) {
        av_frame_free(&videoFrame);
    }
}

bool FFMpegReader::extractSPSPPS(AVCodecParameters *codecParams, std::vector<uint8_t> &sps,
                                 std::vector<uint8_t> &pps) {
    if (!codecParams || !codecParams->extradata || codecParams->extradata_size < 7) {
        LOGE("Invalid extradata");
        return false;
    }
    AVFormatContext* fmt;

    const uint8_t* extradata = codecParams->extradata;
    int extradata_size = codecParams->extradata_size;

    // 检查是否为AVCC格式（一般mp4容器采用）
    if (extradata[0] == 1) { // AVCC格式以1开头
        int offset = 5;      // 跳过版本、profile、compatibility、level和长度字段

        // 获取SPS个数,第六个字节[111......]前三个bit默认为111，后五个bit表示接下来的SPS个数
        int numSPS = extradata[offset] & 0x1F;
        offset++;
        for (int i = 0; i < numSPS; ++i) { //
            // 读取当前SPS长度, 2字节大端序
            int lengthSPS = (extradata[offset] << 8) | extradata[offset + 1];
            offset += 2;

            // 读取SPS, 目前只保留第一个SPS
            if (i == 0) {
                sps.assign(extradata + offset, extradata + offset + lengthSPS);
            }
            offset += lengthSPS;
        }

        int numPPS = extradata[offset] & 0x1F;
        offset++;

        for (int i = 0; i < numPPS; ++i) {
            // 读取当前PPS长度, 2字节大端序
            int lengthPPS = (extradata[offset] << 8) | extradata[offset + 1];
            offset += 2;

            if (i == 0) {
                pps.assign(extradata + offset, extradata + offset + lengthPPS);
            }
            offset += lengthPPS;
        }

        return !sps.empty() && ! pps.empty();
    } else if (extradata[0] == 0 && extradata[1] == 0 &&
               extradata[2] == 0 && extradata[3] == 1){ // 格式为Annex-B(开头是起始码0x00000001)


    }

    return false;
}
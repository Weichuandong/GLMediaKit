//
// Created by Weichuandong on 2025/3/26.
//

#include "Reader/FFMpegVideoReader.h"


FFMpegVideoReader::FFMpegVideoReader(std::shared_ptr<SafeQueue<AVFrame *>> videoFrameQueue,
                                     std::shared_ptr<SafeQueue<AVFrame *>> audioFrameQueue,
                                     ReaderType type) :
    videoFrameQueue(std::move(videoFrameQueue)),
    audioFrameQueue(std::move(audioFrameQueue)),
    readerType(type),
    demuxer(std::make_unique<FFmpegDemuxer>()),
    packet(av_packet_alloc()),
    audioFrame(av_frame_alloc()),
    videoFrame(av_frame_alloc())
{

}

FFMpegVideoReader::~FFMpegVideoReader() {
    release();
}

bool FFMpegVideoReader::open(const std::string &file_path) {
    filePath = file_path;
    if (!demuxer->open(filePath)) {
        LOGE("failed to open file : %s", file_path.c_str());
        return false;
    }

    if (hasAudio()) {
        audioDecoder = std::make_unique<FFMpegAudioDecoder>();
        if (!audioDecoder->configure(demuxer->getAudioCodecParameters())) {
            LOGE("failed to configure audioDecoder");
            return false;
        }
    }

    if (hasVideo()) {
        videoDecoder = std::make_unique<FFmpegVideoDecoder>();
        if (!videoDecoder->configure(demuxer->getVideoCodecParameters())) {
            LOGE("failed to configure videoDecoder");
            return false;
        }
    }
    return true;
}

void FFMpegVideoReader::start() {
    if (isRunning()) return;
    LOGI("FFMpegVideoReader : start read thread");
    exitRequested = false;
    isPaused = false;
    isReady = true;

    readThread = std::thread(&FFMpegVideoReader::readThreadFunc, this);
}

void FFMpegVideoReader::pause() {
    isPaused = true;
}

void FFMpegVideoReader::resume() {
    isPaused = false;
    pauseCond.notify_one();
}

void FFMpegVideoReader::stop() {
    exitRequested =  true;
    resume();

    if (readThread.joinable()) {
        readThread.join();
    }
}

void FFMpegVideoReader::seekTo(double position) {

}

bool FFMpegVideoReader::hasVideo() const {
    return demuxer->hasVideo();
}

bool FFMpegVideoReader::hasAudio() const {
    return demuxer->hasAudio();
}

AVRational FFMpegVideoReader::getAudioTimeBase() const {
    return demuxer->getAudioTimeBase();
}

AVRational FFMpegVideoReader::getVideoTimeBase() const {
    return demuxer->getVideoTimeBase();
}

void FFMpegVideoReader::readThreadFunc() {
    if (!isReadying()) {
        LOGE("Reader is not ready, exit readThread");
        return;
    }

    auto lastLogTime = std::chrono::steady_clock::now();
    uint16_t audioPacketCount = 0;
    uint16_t audioFrameCount = 0;
    uint16_t videoPacketCount = 0;
    uint16_t videoFrameCount = 0;

    while (!exitRequested) {
        {
            // 是否暂停
            std::unique_lock<std::mutex> lk(mtx);
            while (isPaused && !exitRequested) {
                pauseCond.wait(lk);
            }
        }

        if (exitRequested) break;

        {
            PerformanceTimer timer2("demux + decode");
            // 获取packet
            FFmpegDemuxer::PacketType type = demuxer->ReceivePacket(packet);
            if (type == FFmpegDemuxer::PacketType::NONE) {
                continue;
            } else if (type == FFmpegDemuxer::PacketType::AUDIO) {
                audioPacketCount++;
                if (audioDecoder->SendPacket(packet) == 0) {
                    while (audioDecoder->ReceiveFrame(audioFrame) == 0) {
                        PerformanceTimer timer22("audio receive frame + push");
                        LOGI("audio 1");
                        AVFrame* cloneFrame = av_frame_clone(audioFrame);
                        if (!audioFrameQueue->push(cloneFrame)) {
                            LOGE("Failed to push frame to audioFrameQueue");
                            av_frame_unref(cloneFrame);
                        }
                        audioFrameCount++;
                        LOGI("audio 2");
                        av_frame_unref(audioFrame);
                    }
                }
            } else if (type == FFmpegDemuxer::PacketType::VIDEO) {
                videoPacketCount++;
                if(videoDecoder->SendPacket(packet) == 0) {
                    while (videoDecoder->ReceiveFrame(videoFrame) == 0) {
                        AVFrame* cloneFrame = av_frame_clone(videoFrame);
                        if (!videoFrameQueue->push(cloneFrame)) {
                            LOGE("Failed to push frame to videoFrameQueue");
                            av_frame_unref(cloneFrame);
                        }
                        videoFrameCount++;
                        av_frame_unref(videoFrame);
                    }
                }
            }
        }

        av_packet_unref(packet);
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastLogTime).count();
        if (elapsed >= 3000) {
            LOGI("音频解封装统计: %d帧/%.3f秒 (%.2f帧/秒)",
                 audioPacketCount, elapsed / 1000.0,
                 audioPacketCount / (elapsed / 1000.0f));
            LOGI("音频解码统计: %d帧/%.3f秒 (%.2f帧/秒), 队列大小: %d",
                 audioFrameCount, elapsed / 1000.0,
                 audioFrameCount / (elapsed / 1000.0f), audioFrameQueue->getSize());
            LOGI("视频解封装统计: %d帧/%.3f秒 (%.2f帧/秒)",
                 videoPacketCount, elapsed / 1000.0,
                 videoPacketCount / (elapsed / 1000.0f));
            LOGI("视频解码统计: %d帧/%.3f秒 (%.2f帧/秒), 队列大小: %d",
                 videoFrameCount, elapsed / 1000.0,
                 videoFrameCount / (elapsed / 1000.0f), videoFrameQueue->getSize());
            lastLogTime = now;
            audioPacketCount = audioFrameCount = videoPacketCount = videoFrameCount = 0;
        }
    }
}

void FFMpegVideoReader::release() {
    if (packet) {
        av_packet_free(&packet);
    }

    if (audioFrame) {
        av_frame_free(&audioFrame);
    }

    if (videoFrame) {
        av_frame_free(&videoFrame);
    }
}

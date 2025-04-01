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
        audioDecoder = std::make_unique<FFMpegAudioDecoder>();
        if (!audioDecoder->configure(audioDemuxer->getCodecParameters())) {
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
        if (!videoDecoder->configure(videoDemuxer->getCodecParameters())) {
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
        if (audioDecoder->SendPacket(audioPacket) == 0) {
            while (audioDecoder->ReceiveFrame(audioFrame) == 0) {
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
            if (videoDecoder->SendPacket(videoPacket) == 0) {
                while (videoDecoder->ReceiveFrame(videoFrame) == 0) {
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
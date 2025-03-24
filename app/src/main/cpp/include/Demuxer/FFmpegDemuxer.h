//
// Created by Weichuandong on 2025/3/20.
//

#ifndef GLMEDIAKIT_FFMPEGDEMUXER_H
#define GLMEDIAKIT_FFMPEGDEMUXER_H

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/time.h"
};
#include <memory>
#include <thread>
#include <android/log.h>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include "core/SafeQueue.hpp"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "FFmpegDemuxer", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "FFmpegDemuxer", __VA_ARGS__)

class FFmpegDemuxer {
public:
    FFmpegDemuxer(std::shared_ptr<SafeQueue<AVPacket*>> videoPacketQueue,
                  std::shared_ptr<SafeQueue<AVPacket*>> audioPacketQueue);
    ~FFmpegDemuxer();

    bool open(const std::string& filePath);

    // 控制
    void start();
    void pause();
    void resume();
    void stop();
    void seekTo(double position);

    double getDuration() const { return duration; };

    // for decoder
    AVCodecParameters* getVideoCodecParameters();
    AVCodecParameters* getAudioCodecParameters();

    bool isRunning() const { return !exitRequested && demuxingThread.joinable(); }

    bool hasVideo() const;
    bool hasAudio() const;
private:
    AVFormatContext* fmt_ctx;

    int videoStreamIdx;
    int audioStreamIdx;

    std::shared_ptr<SafeQueue<AVPacket*>> videoPacketQueue;
    std::shared_ptr<SafeQueue<AVPacket*>> audioPacketQueue;

    // 线程
    std::thread demuxingThread;

    // 状态
    std::atomic<bool> isPaused;
    std::atomic<bool> exitRequested;
    std::atomic<bool> isEOF;
    std::atomic<bool> isSeekRequested;
    std::mutex mtx;
    std::condition_variable pauseCond;

    double seekPosition;

    std::string filePath;
    double duration;

    void demuxingThreadFunc();

//    int readPacket(void* opaque, uint8_t* buf, int buf_size);
//    int64_t seekPacket(void* opaque, int64_t offset, int whence);
};
#endif //GLMEDIAKIT_FFMPEGDEMUXER_H

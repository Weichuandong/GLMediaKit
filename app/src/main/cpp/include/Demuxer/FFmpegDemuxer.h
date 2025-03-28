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
#include "core/PerformceTimer.hpp"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "FFmpegDemuxer", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "FFmpegDemuxer", __VA_ARGS__)

class FFmpegDemuxer {
public:
    enum struct DemuxerType { AUDIO, VIDEO, NONE};

    FFmpegDemuxer(DemuxerType type = DemuxerType::AUDIO);
    ~FFmpegDemuxer();

    bool open(const std::string& filePath);

    // 控制
    void seekTo(double position);

    int ReceivePacket(AVPacket* packet);

    double getDuration() const { return duration; };

    // for decoder
    AVCodecParameters* getCodecParameters();
//    AVCodecParameters* getAudioCodecParameters();

    bool isReadying() const { return isReady; }
    bool hasVideo() const;
    bool hasAudio() const;
    AVRational getTimeBase() const;
//    AVRational getVideoTimeBase() const;

private:
    AVFormatContext* fmt_ctx;

//    int videoStreamIdx;
//    int audioStreamIdx;
    int streamIdx;

    // 状态
    std::atomic<bool> isSeekRequested{false};
    std::atomic<bool> isReady{false};

    double seekPosition{};

    std::string filePath;
    double duration;

    DemuxerType type;

    void release();
};
#endif //GLMEDIAKIT_FFMPEGDEMUXER_H

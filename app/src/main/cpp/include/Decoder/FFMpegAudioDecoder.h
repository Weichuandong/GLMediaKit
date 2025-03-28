//
// Created by Weichuandong on 2025/3/25.
//

#ifndef GLMEDIAKIT_FFMPEGAUDIODECODER_H
#define GLMEDIAKIT_FFMPEGAUDIODECODER_H

#include <android/log.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>

#include "IDecoder.h"
#include "core/SafeQueue.hpp"
#include "core/PerformceTimer.hpp"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "FFMpegAudioDecoder", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "FFMpegAudioDecoder", __VA_ARGS__)

class FFMpegAudioDecoder : public IAudioDecoder{
public:
    FFMpegAudioDecoder();
    ~FFMpegAudioDecoder() override;

    // 使用解码器参数配置解码器
    bool configure(const AVCodecParameters* codecParams) override;

    int SendPacket(const AVPacket* packet) override;

    int ReceiveFrame(AVFrame* frame) override;

    bool isReadying() override { return isReady; }

    int getSampleRate() override { return inSampleRate; }

    int getChannel() override { return inChannel; }

    int getSampleFormat() override { return inSampleFormat; }

private:
    AVCodecContext* avCodecContext{nullptr};

    std::atomic<bool> isReady{false};

    // 音频属性
    int inChannel {0};
    int inSampleRate {0};
    AVSampleFormat inSampleFormat {AV_SAMPLE_FMT_NONE};
    int outChannel {2};
    int outSampleRate {44100};
    AVSampleFormat outSampleFormat {AV_SAMPLE_FMT_S16P};

    void release();
};

#endif //GLMEDIAKIT_FFMPEGAUDIODECODER_H

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

#include "interface/IDecoder.h"
#include "core/SafeQueue.hpp"
#include "core/PerformceTimer.hpp"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "FFmpegAudioDecoder", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "FFmpegAudioDecoder", __VA_ARGS__)

class FFmpegAudioDecoder : public IAudioDecoder{
public:
    FFmpegAudioDecoder();
    ~FFmpegAudioDecoder() override;

    // 使用解码器参数配置解码器
    bool configure(const DecoderConfig& codecParams) override;

    int SendPacket(const std::shared_ptr<IMediaPacket>& packet) override;

    int ReceiveFrame(std::shared_ptr<IMediaFrame>& frame) override;

    bool isReadying() override { return isReady; }

    int getSampleRate() override { return inSampleRate; }

    int getChannel() override { return inChannel; }

    SampleFormat getSampleFormat() override;

private:
    AVCodecContext* avCodecContext{nullptr};

    std::atomic<bool> isReady{false};

    // 音频属性
    int inChannel {0};
    int inSampleRate {0};
    AVSampleFormat inSampleFormat {AV_SAMPLE_FMT_NONE};

    void release();
};

#endif //GLMEDIAKIT_FFMPEGAUDIODECODER_H

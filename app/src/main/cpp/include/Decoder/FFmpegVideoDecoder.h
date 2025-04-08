//
// Created by Weichuandong on 2025/3/17.
//

#ifndef GLMEDIAKIT_FFMPEGVIDEODECODER_H
#define GLMEDIAKIT_FFMPEGVIDEODECODER_H

#include <android/log.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

#include "interface/IDecoder.h"
#include "core/SafeQueue.hpp"
#include "core/PerformceTimer.hpp"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "FFmpegVideoDecoder", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "FFmpegVideoDecoder", __VA_ARGS__)

class FFmpegVideoDecoder : public IVideoDecoder {
public:
    FFmpegVideoDecoder();
    ~FFmpegVideoDecoder() override;

    // 使用解码器参数配置解码器
    bool configure(const DecoderConfig& codecParams) override;

    int SendPacket(const std::shared_ptr<IMediaPacket>& packet) override;

    int ReceiveFrame(std::shared_ptr<IMediaFrame>& frame) override;

    bool isReadying() override { return isReady; }

    int getWidth() override { return mWidth; }

    int getHeight() override { return mHeight; }

    PixFormat getPixFormat() override;

private:
    AVCodecContext* avCodecContext{nullptr};

    // 状态
    std::atomic<bool> isReady{false};

    // 视频属性
    int mWidth;
    int mHeight;
    AVPixelFormat format;

    void release();
};

#endif //GLMEDIAKIT_FFMPEGVIDEODECODER_H

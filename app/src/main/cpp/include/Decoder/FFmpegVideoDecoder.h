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

#include "IVideoDecoder.h"
#include "core/SafeQueue.hpp"
#include "core/PerformceTimer.hpp"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "FFmpegVideoDecoder", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "FFmpegVideoDecoder", __VA_ARGS__)

class FFmpegVideoDecoder : public IVideoDecoder {
public:

    FFmpegVideoDecoder(std::shared_ptr<SafeQueue<AVPacket*>> pktQueue,
                       std::shared_ptr<SafeQueue<AVFrame*>> frameQueue);
    ~FFmpegVideoDecoder() override;


    // 使用解码器参数配置解码器
    bool configure(const AVCodecParameters* codecParams) override;

    void start() override;

    void pause() override;

    void resume() override;

    void stop() override;

    bool isRunning() override { return !isPaused && videoDecodeThread.joinable(); }
    bool isReadying() override { return isReady; }
    int getWidth() override { return mWidth; }
    int getHeight() override { return mHeight; }
    int getSampleRate() override { return -1; };
    int getChannel() override { return -1; };
    int getSampleFormat() override { return -1; };
private:
    AVCodecContext* avCodecContext{nullptr};

    AVRational timeBase;
    double lastValidPts;

    // 解码线程
    std::thread videoDecodeThread;
    void videoDecodeThreadFunc();

    // 状态
    std::atomic<bool> isPaused{false};
    std::atomic<bool> exitRequested{false};
    std::atomic<bool> isReady{false};
    std::mutex mtx;
    std::condition_variable pauseCond;

    // 数据
    std::shared_ptr<SafeQueue<AVPacket*>> videoPackedQueue;
    std::shared_ptr<SafeQueue<AVFrame*>> videoFrameQueue;

    // 视频属性
    int mWidth;
    int mHeight;
    AVPixelFormat format;

    // 特殊包检查
    bool isSpecialPacket(const AVPacket* packet) const;

    // 处理特殊包（flush或EOF）
    void handleSpecialPacket(AVPacket* packet);

};

#endif //GLMEDIAKIT_FFMPEGVIDEODECODER_H

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

#include "IVideoDecoder.h"
#include "core/SafeQueue.hpp"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "FFMpegAudioDecoder", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "FFMpegAudioDecoder", __VA_ARGS__)

class FFMpegAudioDecoder : public IVideoDecoder{
public:
    FFMpegAudioDecoder(std::shared_ptr<SafeQueue<AVPacket*>> pktQueue,
                       std::shared_ptr<SafeQueue<AVFrame*>> frameQueue);
    ~FFMpegAudioDecoder() override;

    // 使用解码器参数配置解码器
    bool configure(const AVCodecParameters* codecParams) override;

    void start() override;

    void pause() override;

    void resume() override;

    void stop() override;

    bool isRunning() override { return !isPaused && audioDecodeThread.joinable(); }
    bool isReadying() override { return isReady; }

    int getWidth() override { return -1; };

    int getHeight() override { return -1; };

    int getSampleRate() override { return inSampleRate; }

    int getChannel() override { return inChannel; }

    int getSampleFormat() override { return inSampleFormat; }

private:
    AVCodecContext* avCodecContext{nullptr};

    // 解码线程
    std::thread audioDecodeThread;
    void audioDecodeThreadFunc();

    // 状态
    std::atomic<bool> isPaused{false};
    std::atomic<bool> exitRequested{false};
    std::atomic<bool> isReady{false};
    std::mutex mtx;
    std::condition_variable pauseCond;

    // 数据
    std::shared_ptr<SafeQueue<AVPacket*>> audioPackedQueue;
    std::shared_ptr<SafeQueue<AVFrame*>> audioFrameQueue;

    // 音频属性
    int inChannel;
    int inSampleRate;
    AVSampleFormat inSampleFormat;
    int outChannel{2};
    int outSampleRate{44100};
    AVSampleFormat outSampleFormat{AV_SAMPLE_FMT_S16P};
};

#endif //GLMEDIAKIT_FFMPEGAUDIODECODER_H

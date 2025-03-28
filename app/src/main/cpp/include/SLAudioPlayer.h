//
// Created by Weichuandong on 2025/3/25.
//

#ifndef GLMEDIAKIT_SLAUDIOPLAYER_H
#define GLMEDIAKIT_SLAUDIOPLAYER_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
};

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <android/log.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "core/SafeQueue.hpp"
#include "core/IClock.h"
#include "core/MediaSynchronizer.hpp"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "SLAudioPlayer", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "SLAudioPlayer", __VA_ARGS__)

class SLAudioPlayer {
public:
    explicit SLAudioPlayer(std::shared_ptr<SafeQueue<AVFrame*>> frameQueue,
                           std::shared_ptr<MediaSynchronizer> sync);
    ~SLAudioPlayer();

    // 初始化OpenSL ES并设置音频参数
    bool prepare(int sampleRate, int channels, AVSampleFormat format);

    // 控制播放
    void start();
    void pause();
    void resume();
    void stop();
    void release();

    // 音量控制
    void setVolume(float vol);
    float getVolume() const { return volume.load(); }

    // 重置重采样缓冲区
    void resetResampleBuffer() { availableSamples = 0; }
    bool isReadying() { return isReady; }

    void setTimeBase(const AVRational& timeBase);
private:
    // OpenSLES 对象
    SLObjectItf engineObj{nullptr};
    SLEngineItf engine{nullptr};
    SLObjectItf outputMixObj{nullptr};
    SLObjectItf playerObj{nullptr};
    SLPlayItf player{nullptr};
    SLAndroidSimpleBufferQueueItf bufferQueue{nullptr};
    SLVolumeItf volumeItf{nullptr};

    // 音频缓冲区
    static const int NUM_BUFFERS = 2;
    static const int BUFFER_SIZE = 8192;  // 缓冲区大小，根据延迟要求调整
    uint8_t* audioBuffers[NUM_BUFFERS]{};
    int currentBuffer = 0;

    // 音频数据源
    std::shared_ptr<SafeQueue<AVFrame*>> audioFrameQueue;
    std::mutex mutex;
    std::atomic<bool> isRunning{false};
    std::atomic<bool> isReady{false};
    std::atomic<float> volume{1.0f};

    // 音频参数
    int inSampleRate = 0;
    int inChannels = 0;
    AVSampleFormat inFormat = AV_SAMPLE_FMT_NONE;
    int64_t inChannelLayout = 0;

    int outSampleRate = 44100;  // 输出采样率
    int outChannels = 2;        // 立体声输出
    AVSampleFormat outFormat = AV_SAMPLE_FMT_S16;  // 16位PCM
    int64_t outChannelLayout = AV_CH_LAYOUT_STEREO;

    // 重采样
    SwrContext* swrContext = nullptr;
    uint8_t* resampleBuffer = nullptr;
    int resampleBufferSize = 0;
    int availableSamples = 0;

    // 音频时钟
    IClock audioClock;
    // 时间基
    AVRational AudioTimeBase = {0, 0};
    // 主时钟同步控制
    std::shared_ptr<MediaSynchronizer> synchronizer;

    // 静态回调函数
    static void bufferQueueCallback(SLAndroidSimpleBufferQueueItf bq, void* context);

    // 回调处理函数
    void processBuffer();

    // 填充音频缓冲区
    void fillBuffer(uint8_t* buffer, int size);

    // 从解码帧中提取音频并重采样
    int resampleAudio(AVFrame* frame, uint8_t* outBuffer, int outSize);

    // 应用音量
    void applyVolume(int16_t* buffer, int numSamples);
};

#endif //GLMEDIAKIT_SLAUDIOPLAYER_H

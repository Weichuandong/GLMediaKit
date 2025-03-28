//
// Created by Weichuandong on 2025/3/28.
//

#ifndef GLMEDIAKIT_FFMPEGREADER_H
#define GLMEDIAKIT_FFMPEGREADER_H

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
#include "Demuxer/FFmpegDemuxer.h"
#include "Decoder/FFMpegAudioDecoder.h"
#include "Decoder/FFmpegVideoDecoder.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "FFmpegReader", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "FFmpegReader", __VA_ARGS__)

class FFMpegReader {
public:
    enum struct ReaderType { ONLY_VIDEO, ONLY_AUDIO, AUDIO_VIDEO};

    FFMpegReader(std::shared_ptr<SafeQueue<AVFrame*>> videoFrameQueue,
                      std::shared_ptr<SafeQueue<AVFrame*>> audioFrameQueue,
                      ReaderType type = ReaderType::AUDIO_VIDEO);
    ~FFMpegReader();

    bool open(const std::string& filePath);

    // 控制
    void start();
    void pause();
    void resume();
    void stop();
    void seekTo(double position);

    double getDuration() const { return audioDemuxer->getDuration(); };
    bool isRunning() const { return !exitRequested && (audioReadThread.joinable() || videoReadThread.joinable()); }
    bool isReadying() const { return isReady; }
    bool hasVideo() const;
    bool hasAudio() const;
    AVRational getAudioTimeBase() const;
    AVRational getVideoTimeBase() const;
    int getVideoWidth() const { return videoDecoder->getWidth(); }
    int getVideoHeight() const { return videoDecoder->getHeight(); }
    int getSampleRate() const { return audioDecoder->getSampleRate(); }
    int getChannel() const { return audioDecoder->getChannel(); }
    int getSampleFormat() const { return audioDecoder->getSampleFormat(); }

private:
    // 线程
    std::thread audioReadThread;
    std::thread videoReadThread;

    // 状态
    std::atomic<bool> isPaused{false};
    std::atomic<bool> exitRequested{false};
    std::atomic<bool> isSeekRequested{false};
    std::atomic<bool> isReady{false};

    std::mutex audioMtx;
    std::mutex videoMtx;
    std::condition_variable audioPauseCond;
    std::condition_variable videoPauseCond;

    double seekPosition{};

    std::string filePath;
    double duration;

    // 组件
    std::unique_ptr<FFmpegDemuxer> audioDemuxer;
    std::unique_ptr<FFmpegDemuxer> videoDemuxer;
    std::unique_ptr<IAudioDecoder> audioDecoder;
    std::unique_ptr<IVideoDecoder> videoDecoder;

    // 数据
    AVPacket* audioPacket;
    AVPacket* videoPacket;
    AVFrame* audioFrame;
    AVFrame* videoFrame;
    std::shared_ptr<SafeQueue<AVFrame*>> audioFrameQueue;
    std::shared_ptr<SafeQueue<AVFrame*>> videoFrameQueue;

    ReaderType readerType;

    void audioReadThreadFunc();
    void videoReadThreadFunc();

    void releaseAudio();
    void releaseVideo();
};

#endif //GLMEDIAKIT_FFMPEGREADER_H

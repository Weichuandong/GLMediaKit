//
// Created by Weichuandong on 2025/3/19.
//

#ifndef GLMEDIAKIT_PLAYER_H
#define GLMEDIAKIT_PLAYER_H

#include "EGL/EGLCore.h"
#include "Renderer/IRenderer.h"
#include "Decoder/IVideoDecoder.h"
#include "RenderThread.h"
#include "Renderer/VideoRenderer.h"
#include "Decoder/FFmpegVideoDecoder.h"
#include "Decoder/FFMpegAudioDecoder.h"
#include "core/SafeQueue.hpp"
#include "Demuxer/FFmpegDemuxer.h"
#include "SLAudioPlayer.h"
#include "core/MediaSynchronizer.hpp"
#include "Reader/FFMpegVideoReader.h"

#include <memory>
#include <android/log.h>
#include <mutex>
#include <condition_variable>
#include <string>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "Player", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "Player", __VA_ARGS__)

class Player {
public:
    enum struct PlayerState {
        INIT,           // 初始状态
        PREPARED,       // 已准备好
        PLAYING,        // 正在播放
        SEEKING,        //
        PAUSED,         // 已暂停
        STOPPED,        // 已停止
        COMPLETED,      // 播放完成
        ERROR,          // 错误状态
    };

    Player();
    ~Player();

    bool init();

    // 控制方法
    bool prepare(const std::string& filePath);
    bool playback();
    bool pause();
    bool stop();
    bool release();
    bool seekTo(double position);
    bool resume();
//
    // 状态查询
    Player::PlayerState getPlayerState();
    double getDuration() const;
    bool isPlaying() const;

    // 获取视频信息
    int getVideoWidth() const;
    int getVideoHeight() const;

    //
    void attachSurface(ANativeWindow* window);
    void detachSurface();
    void surfaceSizeChanged(int width, int height);

    // 音量控制

private:
    //
    std::unique_ptr<EGLCore> eglCore;
    std::unique_ptr<IRenderer> renderer;
    std::unique_ptr<RenderThread> renderThread;
    std::unique_ptr<SLAudioPlayer> audioPlayer;
    std::unique_ptr<FFMpegVideoReader> reader;
    std::shared_ptr<MediaSynchronizer> synchronizer;

    PlayerState currentState;
    PlayerState previousState; // 用于Seeking后恢复
    std::mutex stateMtx;
    std::condition_variable stateCond;

    bool isAttachSurface;
    std::mutex attachSurfaceMtx;
    std::condition_variable attachSurfaceCond;

    std::string mediaPath{""};
    double seekPosition{};
    std::atomic<bool> fileChanged{false};
    // 相关队列
    std::shared_ptr<SafeQueue<AVFrame*>> videoFrameQueue;
    std::shared_ptr<SafeQueue<AVFrame*>> audioFrameQueue;

    // 状态转换方法
    void changeState(PlayerState newState);
    bool canTransitionTo(PlayerState targetState);

    // 组件控制方法
    void resetComponents();
    void startMediaLoading();
    void prepareDecoders();
    void startPlayback();
    void pausePlayback();
    void performSeeking();
    void handleCompletion();
    void handleError();
    void releaseResources();

    // 禁止拷贝构造和赋值
    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;
};
#endif //GLMEDIAKIT_PLAYER_H

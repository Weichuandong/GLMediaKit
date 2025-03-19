//
// Created by Weichuandong on 2025/3/19.
//

#ifndef GLMEDIAKIT_PLAYER_H
#define GLMEDIAKIT_PLAYER_H

#include "EGL/EGLCore.h"
#include "Renderer/IRenderer.h"
#include "Decoder/IVideoDecoder.h"
#include "RenderThread.h"
#include "Renderer/GLRenderer.h"
#include "Decoder/FFmpegVideoDecoder.h"

#include <memory>
#include <android/log.h>

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

    // prepare
    void prepare(std::string filePath);

    void playback();

    void pause();

    void stop();

    void release();

    Player::PlayerState getPlayerState();

    void attachSurface(ANativeWindow* window);

    void detachSurface();

    void surfaceSizeChanged(int width, int height);

private:

    std::unique_ptr<EGLCore> eglCore;

    std::shared_ptr<IRenderer> renderer;

    std::unique_ptr<IVideoDecoder> videoDecoder;

    std::unique_ptr<RenderThread> renderThread;

    PlayerState state;

    bool isAttachSurface;
};
#endif //GLMEDIAKIT_PLAYER_H

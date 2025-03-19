//
// Created by Weichuandong on 2025/3/19.
//

#include "Player.h"


Player::Player():
    eglCore(std::make_unique<EGLCore>()),
    renderer(std::make_shared<GLRenderer>()),
    videoDecoder(std::make_unique<FFmpegVideoDecoder>()),
    renderThread(std::make_unique<RenderThread>()),
    state(PlayerState::INIT),
    isAttachSurface(false)
{
    init();
}

Player::~Player() {

}

bool Player::init() {
    if (!eglCore->init()) {
        LOGE("Failed to initialize EGLCore");
        return false;
    }

    return true;
}

void Player::prepare(std::string filePath) {

}

void Player::playback() {
    LOGI("Start playback");
    if (!isAttachSurface) {
        LOGE("not attach surface before playback");
        return;
    }
    // 启动渲染线程
    if (!renderThread->isRunning()) {
        renderThread->start(renderer.get(), eglCore.get());
    }
}

void Player::pause() {

}

void Player::stop() {

}

Player::PlayerState Player::getPlayerState() {
    return state;
}

void Player::attachSurface(ANativeWindow* window) {
    if (eglCore->createWindowSurface(window)) {
        isAttachSurface = true;
        return;
    }
    LOGE("Failed to attach Surface");
}

void Player::detachSurface() {
    LOGI("Surface detach");

    // 停止渲染线程
    renderThread->stop();

    // 释放EGL表面
    if (eglCore) {
        eglCore->destroySurface();
    }
}

void Player::surfaceSizeChanged(int width, int height) {


    renderer->onSurfaceChanged(width, height);

}

void Player::release() {
    LOGI("MediaManager release.");
    // 停止渲染线程
    if (renderThread) {
        renderThread->stop();
    }

    // 释放EGL资源
    if (eglCore) {
        eglCore->destroySurface();
    }
}

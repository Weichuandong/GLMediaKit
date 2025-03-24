//
// Created by Weichuandong on 2025/3/19.
//

#include "Player.h"


Player::Player():
    videoPacketQueue(std::make_shared<SafeQueue<AVPacket*>>()),
    audioPacketQueue(std::make_shared<SafeQueue<AVPacket*>>()),
    videoFrameQueue(std::make_shared<SafeQueue<AVFrame*>>()),
    eglCore(std::make_unique<EGLCore>()),
    renderThread(std::make_unique<RenderThread>()),
    currentState(PlayerState::INIT),
    previousState(PlayerState::INIT),
    isAttachSurface(false)
{
    renderer = std::make_shared<VideoRenderer>(videoFrameQueue);

    init();
}

Player::~Player() {
    release();
}

bool Player::init() {
    if (!eglCore->init()) {
        LOGE("Failed to initialize EGLCore");
        return false;
    }

    return true;
}

bool Player::prepare(const std::string& filePath) {
    LOGI("Player prepare, filePath = %s", filePath.c_str());
    mediaPath = filePath;

    if (!canTransitionTo((PlayerState::PREPARED))) {
        return false;
    }

    changeState(PlayerState::PREPARED);
    return true;
}

bool Player::playback() {
    LOGI("Start playback");
    // 等待Surface创建并attach
    {
        std::unique_lock<std::mutex> lk(attachSurfaceMtx);
        attachSurfaceCond.wait(lk, [this]() { return isAttachSurface; });
    }
    if (!isAttachSurface) {
        LOGE("not attach surface before playback");
        return false;
    }
    if (!canTransitionTo(PlayerState::PLAYING)) {
        return false;
    }
    changeState(PlayerState::PLAYING);
    return true;
}

bool Player::pause() {
    LOGI("Player pause");

    if (!canTransitionTo(PlayerState::PAUSED)) {
        return false;
    }
    changeState(PlayerState::PAUSED);
    return true;
}

bool Player::stop() {
    LOGI("Player stopped");

    if (!canTransitionTo(PlayerState::STOPPED)) {
        return false;
    }

    changeState(PlayerState::STOPPED);
    return true;
}


bool Player::seekTo(double position) {
    LOGI("Player seekTo : %f", position);
    seekPosition = position;
    if (!canTransitionTo(PlayerState::SEEKING)) {
        return false;
    }
    changeState(PlayerState::SEEKING);
    return true;
}

Player::PlayerState Player::getPlayerState() {
    return currentState;
}

void Player::attachSurface(ANativeWindow* window) {
    LOGI("Player attach to a surface");
    if (eglCore->createWindowSurface(window)) {
        std::lock_guard<std::mutex> lk(attachSurfaceMtx);
        isAttachSurface = true;
        attachSurfaceCond.notify_one();
        return;
    }
    LOGE("Failed to attach Surface");
}

void Player::detachSurface() {
    LOGI("Surface detach");

    isAttachSurface = false;

    // 释放EGL表面
    if (eglCore) {
        eglCore->destroySurface();
    }

    // 停止处理
    if (!canTransitionTo(PlayerState::INIT)) {
        return;
    }
    changeState(PlayerState::INIT);
}

void Player::surfaceSizeChanged(int width, int height) {
    LOGI("SurfaceSize Changed");
    renderer->onSurfaceChanged(width, height);
}

bool Player::release() {
    LOGI("Player release.");
    // 停止渲染线程
    if (renderThread) {
        renderThread->stop();
    }

    // 释放EGL资源
    if (eglCore) {
        eglCore->destroySurface();
    }
}

void Player::changeState(Player::PlayerState newState) {
    PlayerState oldState;
    {
        std::lock_guard<std::mutex> lock(stateMtx);
        oldState = currentState;

        if (oldState == newState) return;

        // 记录前一个状态（用于SEEKING后恢复）
        if (newState == PlayerState::SEEKING) {
            previousState = oldState;
        }

        currentState = newState;
    }

    // 执行状态转换时的相关操作
    switch (newState) {
        case PlayerState::INIT:
            resetComponents();
            break;

        case PlayerState::PREPARED:
            startMediaLoading();
            prepareDecoders();
            break;

        case PlayerState::PLAYING:
            startPlayback();
            break;

        case PlayerState::PAUSED:
            pausePlayback();
            break;

        case PlayerState::SEEKING:
            performSeeking();
            break;

        case PlayerState::COMPLETED:
            handleCompletion();
            break;

        case PlayerState::ERROR:
            handleError();
            break;

        case PlayerState::STOPPED:
            releaseResources();
            break;
    }
}

bool Player::canTransitionTo(Player::PlayerState targetState) {
    std::lock_guard<std::mutex> lk(stateMtx);

    switch (currentState) {
        case PlayerState::INIT:
            return targetState == PlayerState::PREPARED ||
                   targetState == PlayerState::STOPPED;

        case PlayerState::PREPARED:
            return targetState == PlayerState::PLAYING ||
                   targetState == PlayerState::PAUSED ||
                   targetState == PlayerState::STOPPED;

        case PlayerState::PLAYING:
            return targetState == PlayerState::PAUSED ||
                   targetState == PlayerState::SEEKING ||
                   targetState == PlayerState::COMPLETED ||
                   targetState == PlayerState::ERROR ||
                   targetState == PlayerState::STOPPED;

        case PlayerState::PAUSED:
            return targetState == PlayerState::PLAYING ||
                   targetState == PlayerState::SEEKING ||
                   targetState == PlayerState::ERROR ||
                   targetState == PlayerState::STOPPED;

        case PlayerState::SEEKING:
            return targetState == PlayerState::PLAYING ||
                   targetState == PlayerState::PAUSED ||
                   targetState == PlayerState::ERROR ||
                   targetState == PlayerState::STOPPED;

        case PlayerState::COMPLETED:
            return targetState == PlayerState::PLAYING ||
                   targetState == PlayerState::PREPARED ||
                   targetState == PlayerState::STOPPED;

        case PlayerState::ERROR:
            return targetState == PlayerState::INIT ||
                   targetState == PlayerState::STOPPED;

        case PlayerState::STOPPED:
            return false;
    }

    return false;
}

void Player::resetComponents() {
    // 重置组件（
    demuxer.reset();
    videoDecoder.reset();
//    renderThread.reset();
//    renderer.reset();

    // 重置队列
    videoPacketQueue = std::make_shared<SafeQueue<AVPacket*>>();
    audioPacketQueue = std::make_shared<SafeQueue<AVPacket*>>();
    videoFrameQueue = std::make_shared<SafeQueue<AVFrame*>>();

}

void Player::startMediaLoading() {
    demuxer = std::make_unique<FFmpegDemuxer>(videoPacketQueue, audioPacketQueue);

    demuxer->open(mediaPath);

}

void Player::prepareDecoders() {
    if (!videoDecoder && demuxer->hasVideo()) {
        videoDecoder = std::make_unique<FFmpegVideoDecoder>(videoPacketQueue, videoFrameQueue);
        if (!videoDecoder->configure(demuxer->getVideoCodecParameters())) {
            LOGE("Failed to configure videoDecoder");
            changeState(PlayerState::ERROR);
            return;
        }
    }

    if (demuxer->hasAudio()) {

    }
}

void prepareRenderer() {

}
void Player::startPlayback() {

    //  启动解封装线程
    if (demuxer && !demuxer->isRunning()) {
        demuxer->start();
    }

    // 启动视频解码线程
    if (videoDecoder && !videoDecoder->isRunning()) {
        videoDecoder->start();
    }

    // 启动渲染线程
    if (renderThread && !renderThread->isRunning()) {
        renderThread->start(renderer.get(), eglCore.get());
    }
}

void Player::pausePlayback() {
    if (demuxer) {
        demuxer->pause();
    }

    if (videoDecoder) {
        videoDecoder->pause();
    }

    if (renderThread) {
        renderThread->pause();
    }
}

void Player::performSeeking() {
    pausePlayback();

    demuxer->seekTo(seekPosition);

    if (previousState == PlayerState::PLAYING) {
        changeState(PlayerState::PLAYING);
    } else {
        changeState(PlayerState::PAUSED);
    }
}

void Player::handleCompletion() {
    pausePlayback();
}

void Player::handleError() {
    LOGE("error");
}

void Player::releaseResources() {
    LOGI("Player releaseResources");
    if (renderThread) {
        renderThread->stop();
    }

    if (videoDecoder) {
        videoDecoder->stop();
    }

    if (demuxer) {
        demuxer->stop();
    }

    videoPacketQueue->flush();
    videoFrameQueue->flush();
    audioPacketQueue->flush();

    renderThread.reset();
    videoDecoder.reset();
    demuxer.reset();
}

double Player::getDuration() const {
    return demuxer->getDuration();
}

//double Player::getCurrentPosition() const {
//    return ;
//}

bool Player::isPlaying() const {
    return currentState == PlayerState::PLAYING;
}

int Player::getVideoWidth() const {
    return videoDecoder->getWidth();
}

int Player::getVideoHeight() const {
    return videoDecoder->getHeight();
}
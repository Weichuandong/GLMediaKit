//
// Created by Weichuandong on 2025/3/19.
//

#include "Player.h"

Player::Player():
    videoFrameQueue(std::make_shared<SafeQueue<AVFrame*>>(10)),
    audioFrameQueue(std::make_unique<SafeQueue<AVFrame*>>(10)),
    synchronizer(std::make_shared<MediaSynchronizer>(MediaSynchronizer::SyncSource::AUDIO)),
    eglCore(std::make_unique<EGLCore>()),
    renderer(std::make_unique<VideoRenderer>()),
    currentState(PlayerState::INIT),
    previousState(PlayerState::INIT),
    isAttachSurface(false)
{
    audioPlayer = std::make_unique<SLAudioPlayer>(audioFrameQueue, synchronizer);
    renderThread = std::make_unique<RenderThread>(videoFrameQueue,synchronizer),
    reader = std::make_unique<FFMpegVideoReader>(videoFrameQueue, audioFrameQueue),

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
    if (!mediaPath.empty()) {
        fileChanged = true;
        videoFrameQueue->flush();
        audioFrameQueue->flush();
        videoFrameQueue->resume();
        audioFrameQueue->flush();
        audioFrameQueue->resume();
    }
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
        while (!isAttachSurface) {
            attachSurfaceCond.wait(lk, [this]() { return isAttachSurface; });
        }
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

bool Player::resume() {
    LOGI("Player resume");

    if (!canTransitionTo(PlayerState::PLAYING)) {
        return false;
    }
    changeState(PlayerState::PLAYING);
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
    if (!canTransitionTo(PlayerState::PAUSED)) {
        return;
    }
    changeState(PlayerState::PAUSED);
}

void Player::surfaceSizeChanged(int width, int height) {
    LOGI("SurfaceSize Changed");
    renderThread->postTask([width, height, this]() {
        renderer->onSurfaceChanged(width, height);
    });
    renderThread->postTask([this]() {
        eglCore->makeCurrent();
    });
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

        // 记录前一个状态
        previousState = oldState;
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
                   targetState == PlayerState::STOPPED ||
                   targetState == PlayerState::INIT;

        case PlayerState::PAUSED:
            return targetState == PlayerState::PLAYING ||
                   targetState == PlayerState::SEEKING ||
                   targetState == PlayerState::ERROR ||
                   targetState == PlayerState::STOPPED ||
                   targetState == PlayerState::INIT ||
                   targetState == PlayerState::PREPARED;

        case PlayerState::SEEKING:
            return targetState == PlayerState::PLAYING ||
                   targetState == PlayerState::PAUSED ||
                   targetState == PlayerState::ERROR ||
                   targetState == PlayerState::STOPPED ||
                   targetState == PlayerState::INIT;

        case PlayerState::COMPLETED:
            return targetState == PlayerState::PLAYING ||
                   targetState == PlayerState::PREPARED ||
                   targetState == PlayerState::STOPPED ||
                   targetState == PlayerState::INIT;

        case PlayerState::ERROR:
            return targetState == PlayerState::INIT ||
                   targetState == PlayerState::STOPPED;

        case PlayerState::STOPPED:
            return false;
    }

    return false;
}

void Player::resetComponents() {
    //
    if (reader) {
        reader->pause();
    }

    if (renderThread) {
        renderThread->pause();
    }

    if (audioPlayer) {
        audioPlayer->pause();
    }
}

void Player::startMediaLoading() {
    if (fileChanged){
        reader->stop();
        reader.reset();
        reader = std::make_unique<FFMpegVideoReader>(videoFrameQueue, audioFrameQueue);
    }
    reader->open(mediaPath);
}

void Player::prepareDecoders() {
    if (!audioPlayer->prepare(reader->getSampleRate(),
                              reader->getChannel(),
                              static_cast<AVSampleFormat>(reader->getSampleFormat()),
                              reader->getAudioTimeBase())) {
        LOGE("audioPlayer prepare failed");
    }
}

void Player::startPlayback() {
    if (previousState == PlayerState::PAUSED) {
        // 暂停后重新播放
        if (reader) reader->resume();
        if (renderThread) renderThread->resume();
        if (audioPlayer) audioPlayer->resume();
    } else {
        // 初次播放
        // 启动解封装线程
        if (reader && !reader->isReadying()) {
            reader->start();
        }

        if (audioPlayer) {
            audioPlayer->start();
        }

        // 启动渲染线程
        if (renderThread && !renderThread->isReadying()) {
            renderThread->start(renderer.get(), eglCore.get(), reader->getVideoTimeBase());
        } else if (renderThread){
            renderThread->resume();
        }
    }
}

void Player::pausePlayback() {
    LOGI("Player pause playback");
    if (reader) {
        reader->pause();
    }

    if (audioPlayer) {
        audioPlayer->pause();
    }

    if (renderThread) {
        renderThread->pause();
    }
}

void Player::performSeeking() {
    pausePlayback();

    reader->seekTo(seekPosition);

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

    if (reader) {
        reader->stop();
    }

    if (audioPlayer) {
        audioPlayer->stop();
    }

    videoFrameQueue->flush();
    audioFrameQueue->flush();

    renderThread.reset();
    audioPlayer.reset();
    reader.reset();
}

double Player::getDuration() const {
    return reader->getDuration();
}

bool Player::isPlaying() const {
    return currentState == PlayerState::PLAYING;
}

int Player::getVideoWidth() const {
    return reader->getVideoWidth();
}

int Player::getVideoHeight() const {
    return reader->getVideoHeight();
}
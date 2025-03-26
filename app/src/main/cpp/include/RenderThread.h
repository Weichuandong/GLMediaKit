//
// Created by Weichuandong on 2025/3/10.
//

#ifndef GLMEDIAKIT_RENDERTHREAD_H
#define GLMEDIAKIT_RENDERTHREAD_H

extern "C" {
#include <libavcodec/avcodec.h>
};
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>

#include "Renderer/IRenderer.h"
#include "EGL/EGLCore.h"
#include "core/SafeQueue.hpp"
#include "core/IClock.h"
#include "core/MediaSynchronizer.hpp"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "RenderThread", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "RenderThread", __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "RenderThread", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, "RenderThread", __VA_ARGS__)

class RenderThread {
public:

    RenderThread(std::shared_ptr<SafeQueue<AVFrame*>> frameQueue,
                 std::shared_ptr<MediaSynchronizer> sync);
    ~RenderThread();

    void start(IRenderer* renderer, EGLCore* eglCore, AVRational rational);
    void stop();
    bool isRunning() const;
    bool isReadying() const;
    void pause();
    void resume();

    void postTask(const std::function<void()>& task);
private:
    std::thread thread;

    IRenderer* renderer;
    EGLCore* eglCore;

    void renderLoop();

    // 状态控制
    std::atomic<bool> isPaused{false};
    std::atomic<bool> exitRequest{false};
    std::atomic<bool> isReady{false};

    std::mutex mtx;
    std::condition_variable pauseCond;

    // GL任务队列
    std::queue<std::function<void()>> glTasks;
    std::mutex taskMtx;
    std::condition_variable taskCond;
    void executeGLTasks();

    // Frame数据
    std::shared_ptr<SafeQueue<AVFrame*>> videoFrameQueue;

    // 视频时间基
    AVRational videoTimeBase{};
    // 视频时钟
    IClock videoClock;
    // 主时钟控制
    std::shared_ptr<MediaSynchronizer> synchronizer;
};


#endif //GLMEDIAKIT_RENDERTHREAD_H

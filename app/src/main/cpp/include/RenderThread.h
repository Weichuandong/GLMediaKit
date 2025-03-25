//
// Created by Weichuandong on 2025/3/10.
//

#ifndef GLMEDIAKIT_RENDERTHREAD_H
#define GLMEDIAKIT_RENDERTHREAD_H
// 添加前向声明
class MediaManager;

#include <thread>
#include <atomic>
#include <mutex>
#include <queue>

#include "Renderer/IRenderer.h"
#include "EGL/EGLCore.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "RenderThread", __VA_ARGS__)

class RenderThread {
public:
    RenderThread();
    ~RenderThread();

    void start(IRenderer* renderer, EGLCore* eglCore);
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
};


#endif //GLMEDIAKIT_RENDERTHREAD_H

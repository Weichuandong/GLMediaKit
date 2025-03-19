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
#include "Renderer/IRenderer.h"
#include "EGL/EGLCore.h"
//#include "MediaManager.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "RenderThread", __VA_ARGS__)

class RenderThread {
public:
    RenderThread();
    ~RenderThread();

    void start(IRenderer* renderer, EGLCore* eglCore);
    void stop();
    bool isRunning() const;

private:
    std::thread thread;
    std::atomic<bool> running;
    std::mutex mutex;

    IRenderer* renderer;
//    EGLSurface eglSurface;
    EGLCore* eglCore;

    // 添加对EGLManager的引用
//    MediaManager* eglManager;
    bool rendererInitialized;

    void renderLoop();
};


#endif //GLMEDIAKIT_RENDERTHREAD_H

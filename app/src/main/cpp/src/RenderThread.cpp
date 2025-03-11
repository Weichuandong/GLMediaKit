//
// Created by Weichuandong on 2025/3/10.
//

#include "RenderThread.h"

RenderThread::RenderThread()
        : running(false),
          renderer(nullptr),
          eglSurface(EGL_NO_SURFACE),
          eglCore(nullptr),
          eglManager(nullptr),
          rendererInitialized(false){
}

RenderThread::~RenderThread() {
    stop();
}

void RenderThread::start(IRenderer* r, EGLSurface surface, EGLCore* core, EGLManager* manager) {
    if (running) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex);
    renderer = r;
    eglSurface = surface;
    eglCore = core;
    running = true;
    eglManager = manager;

    thread = std::thread(&RenderThread::renderLoop, this);
}

void RenderThread::stop() {
    if (!running) {
        return;
    }

    running = false;
    if (thread.joinable()) {
        thread.join();
    }
}

bool RenderThread::isRunning() const {
    return running;
}

void RenderThread::renderLoop() {
    LOGI("Render loop started");

    // 确保在渲染线程中使用EGL上下文
    if(!eglCore->makeCurrent(eglSurface)) {
        LOGE("Can not makeCurrent");
        return;
    }

    if (renderer && !rendererInitialized) {
        if (!renderer->init()) {
            LOGE("Failed to init renderer");
            return;
        }
        rendererInitialized = true;
    }

    while (running) {
        std::lock_guard<std::mutex> lock(mutex);

        // 检查是否处理尺寸变化
        int width, height;
        if (eglManager->checkAndResetSurfaceChanged(width, height)) {
            if (renderer) {
                renderer->onSurfaceChanged(width, height);
            }
        }

        if (renderer) {
            renderer->onDrawFrame();
        }

        // 交换缓冲区
        eglCore->swapBuffers(eglSurface);

        // 控制帧率
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60fps
    }

    LOGI("Render loop stopped");
}
//
// Created by Weichuandong on 2025/3/10.
//

#include "RenderThread.h"

RenderThread::RenderThread()
        : running(false),
          renderer(nullptr),
          eglCore(nullptr),
          rendererInitialized(false){
}

RenderThread::~RenderThread() {
    stop();
}

void RenderThread::start(IRenderer* r, EGLCore* core) {
    if (running) {
        return;
    }
    LOGI("RenderThread : start render thread");

    std::lock_guard<std::mutex> lock(mutex);
    renderer = r;
    eglCore = core;
    running = true;

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
    if(!eglCore->makeCurrent()) {
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
        //判断是否暂停
        {

        }

        std::lock_guard<std::mutex> lock(mutex);
        if (renderer) {
            renderer->onDrawFrame();
        }
        // 交换缓冲区
        eglCore->swapBuffers();
        // 控制帧率
//        std::this_thread::sleep_for(std::chrono::milliseconds(30)); // ~30fps
    }
    LOGI("Render loop stopped");
}

void RenderThread::pause() const {

}

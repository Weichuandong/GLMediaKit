//
// Created by Weichuandong on 2025/3/10.
//

#include "RenderThread.h"

RenderThread::RenderThread()
        : renderer(nullptr),
          eglCore(nullptr) {
}

RenderThread::~RenderThread() {
    stop();
}

void RenderThread::start(IRenderer* r, EGLCore* core) {
    if (exitRequest) {
        return;
    }
    LOGI("RenderThread : start render thread");

    if (!r || !core) {
        LOGE("eglcore or renderer is null");
        return;
    }

    isPaused = false;
    renderer = r;
    eglCore = core;
    exitRequest = false;

    isReady = true;
    thread = std::thread(&RenderThread::renderLoop, this);
}

void RenderThread::stop() {
    if (exitRequest) {
        return;
    }

    exitRequest = true;
    if (thread.joinable()) {
        thread.join();
    }
}

bool RenderThread::isRunning() const {
    return !isPaused && thread.joinable();
}

void RenderThread::renderLoop() {
    LOGI("Render loop started");

    // 确保在渲染线程中使用EGL上下文
    if(!eglCore->makeCurrent()) {
        LOGE("Can not makeCurrent");
        return;
    }

    if (renderer) {
        if (!renderer->init()) {
            LOGE("Failed to init renderer");
            return;
        }
    }

    auto lastLogTime = std::chrono::steady_clock::now();
    uint16_t renderFrameCount = 0;

    while (!exitRequest) {
        //判断是否暂停
        {
            std::unique_lock<std::mutex> lock(mtx);
            while (isPaused && !exitRequest) {
                pauseCond.wait(lock);
            }
        }

        executeGLTasks();

        if (renderer) {
            renderer->onDrawFrame();
        }

        // 判断egl是否有效
        if (eglCore == nullptr || eglCore->getSurface() == EGL_NO_SURFACE) {
            LOGE("当前eglCore无效或未绑定Surface");
            continue;
        }
        // 交换缓冲区
        eglCore->swapBuffers();

        renderFrameCount++;
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastLogTime);
        if (elapsed.count() >= 3) {
            LOGI("渲染统计: %d帧/%ds (%.2f帧/秒)",
                 renderFrameCount, (int)elapsed.count(),
                 renderFrameCount/(float)elapsed.count());
            renderFrameCount = 0;
            lastLogTime = now;
        }

        // 控制帧率
        std::this_thread::sleep_for(std::chrono::milliseconds(30)); // ~30fps
    }
    LOGI("Render loop stopped");
}

void RenderThread::pause() {
    isPaused = true;
}

void RenderThread::resume() {
    isPaused = false;
    pauseCond.notify_one();
}

void RenderThread::executeGLTasks() {
    std::unique_lock<std::mutex> lk(taskMtx);
    while (!glTasks.empty()) {
        auto task = glTasks.front();
        glTasks.pop();

        lk.unlock();
        task();
        lk.lock();
    }
}

// 提交需要在GL线程上执行的任务
void RenderThread::postTask(const std::function<void()>& task) {
    std::lock_guard<std::mutex> lock(taskMtx);
    glTasks.emplace(task);
}

bool RenderThread::isReadying() const {
    return isReady;
}

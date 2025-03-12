//
// Created by Weichuandong on 2025/3/10.
//
#include "EGL/EGLManager.h"
#include "RenderThread.h"

EGLManager::EGLManager()
        : eglCore(nullptr),
          eglSurface(EGL_NO_SURFACE),
          renderer(nullptr),
          window(nullptr),
          width(0),
          height(0),
          renderThread(new RenderThread()),
          surfaceSizeChanged(false){
}

EGLManager::~EGLManager() {
    release();
}

bool EGLManager::init() {
    eglCore = new EGLCore();
    if (!eglCore->init()) {
        LOGE("Failed to initialize EGLCore");
        delete eglCore;
        eglCore = nullptr;
        return false;
    }
    return true;
}

void EGLManager::surfaceCreated(ANativeWindow* nativeWindow) {
    LOGI("Surface created");
    window = nativeWindow;

    // 创建EGL表面
    eglSurface = eglCore->createWindowSurface(window);
    if (eglSurface == EGL_NO_SURFACE) {
        LOGE("Failed to create EGL surface");
        return;
    }

    // 创建渲染器
    if (renderer == nullptr) {
//        renderer = new ImageRenderer();
        renderer = new GLRenderer();
    }
}

void EGLManager::surfaceChanged(int w, int h) {
    LOGI("Surface changed: %d x %d", w, h);

    {
        std::lock_guard<std::mutex> lock(stateMutex);
        width = w;
        height = h;

        surfaceSizeChanged = true;
    }

    // 启动渲染线程
    if (!renderThread->isRunning()) {
        renderThread->start(renderer, eglSurface, eglCore, this);
    }
}

void EGLManager::surfaceDestroyed() {
    LOGI("Surface destroyed");

    // 停止渲染线程
    renderThread->stop();

    // 释放EGL表面
    if (eglCore && eglSurface != EGL_NO_SURFACE) {
        eglCore->destroySurface(eglSurface);
        eglSurface = EGL_NO_SURFACE;
    }

    // 释放窗口
    if (window) {
        ANativeWindow_release(window);
        window = nullptr;
    }
}

void EGLManager::setImage(void* data, int w, int h) {
    if (renderer) {
        auto* imageRenderer = dynamic_cast<ImageRenderer*>(renderer);
        if (imageRenderer) {
            imageRenderer->setImage(data, w, h);
        }
    }
}

void EGLManager::release() {
    // 停止渲染线程
    if (renderThread) {
        renderThread->stop();
        delete renderThread;
        renderThread = nullptr;
    }
    // 释放渲染器
    if (renderer) {
        renderer->release();
        delete renderer;
        renderer = nullptr;
    }

    // 释放EGL资源
    if (eglCore) {
        if (eglSurface != EGL_NO_SURFACE) {
            eglCore->destroySurface(eglSurface);
            eglSurface = EGL_NO_SURFACE;
        }
        eglCore->release();
        delete eglCore;
        eglCore = nullptr;
    }

    // 释放窗口
    if (window) {
        ANativeWindow_release(window);
        window = nullptr;
    }
}

bool EGLManager::checkAndResetSurfaceChanged(int& w, int& h) {
    std::lock_guard<std::mutex> lock(stateMutex);
    if (surfaceSizeChanged) {
        w = width;
        h = height;
        surfaceSizeChanged = false;
        return true;
    }
    return false;
}

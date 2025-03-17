//
// Created by Weichuandong on 2025/3/10.
//
#include "MediaManager.h"
#include "RenderThread.h"

MediaManager::MediaManager()
        : eglCore(new EGLCore()),
          eglSurface(EGL_NO_SURFACE),
          renderer(new GLRenderer()),
          window(nullptr),
          width(0),
          height(0),
          renderThread(new RenderThread()),
          surfaceSizeChanged(false),
          textureManager(new TextureManager()){
}

MediaManager::~MediaManager() {
    release();
}

bool MediaManager::init() {
    if (!eglCore->init()) {
        LOGE("Failed to initialize EGLCore");
        delete eglCore;
        eglCore = nullptr;
        return false;
    }
    return true;
}

void MediaManager::surfaceCreated(ANativeWindow* nativeWindow) {
    LOGI("Surface created");
    window = nativeWindow;

    // 创建EGL表面
    eglSurface = eglCore->createWindowSurface(window);
    if (eglSurface == EGL_NO_SURFACE) {
        LOGE("Failed to create EGL surface");
        return;
    }
}

void MediaManager::surfaceChanged(int w, int h) {
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

void MediaManager::surfaceDestroyed() {
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

void MediaManager::setImage(void* data, int w, int h) {
    if (renderer) {
        auto* imageRenderer = dynamic_cast<ImageRenderer*>(renderer);
        if (imageRenderer) {
            imageRenderer->setImage(data, w, h);
        }
    }
}

void MediaManager::release() {
    LOGI("MediaManager release.");
    // 停止渲染线程
    if (renderThread) {
        delete renderThread;
        renderThread = nullptr;
    }
    // 释放渲染器
    if (renderer) {
        delete renderer;
        renderer = nullptr;
    }

    // 释放EGL资源
    if (eglCore) {
        if (eglSurface != EGL_NO_SURFACE) {
            eglCore->destroySurface(eglSurface);
            eglSurface = EGL_NO_SURFACE;
        }
        delete eglCore;
        eglCore = nullptr;
    }

    // 释放窗口
    if (window) {
        ANativeWindow_release(window);
        window = nullptr;
    }

    // 释放TextureManager
    if (textureManager){
        textureManager->deleteAllTextures();
        delete textureManager;
    }
}

bool MediaManager::checkAndResetSurfaceChanged(int& w, int& h) {
    std::lock_guard<std::mutex> lock(stateMutex);
    if (surfaceSizeChanged) {
        w = width;
        h = height;
        surfaceSizeChanged = false;
        return true;
    }
    return false;
}

int MediaManager::createTextureFromBitmap(JNIEnv *env, jobject bitmap, jstring key) {
    if (!bitmap) {
        LOGE("Bitmap is null");
    }

    textureManager->createTextureFromBitmap(env, bitmap, reinterpret_cast<const char *>(key));
    return 0;
}

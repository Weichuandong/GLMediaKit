//
// Created by Weichuandong on 2025/3/10.
//

#include "EGL/EGLCore.h"

EGLCore::EGLCore()
    : eglDisplay(EGL_NO_DISPLAY),
      eglContext(EGL_NO_CONTEXT){
}

EGLCore::~EGLCore() {
    release();
}

bool EGLCore::init() {
    // 获取EGL显示连接
    eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (eglDisplay == EGL_NO_DISPLAY) {
        LOGE("Unable to get EGL display");
        return false;
    }

    // 初始化EGL
    EGLint major, minor;
    if (!eglInitialize(eglDisplay, &major, &minor)) {
        LOGE("Unable to initialize EGL");
        return false;
    }
    LOGI("EGL initialize: %d.%d", major, minor);

    // 配置EGL
    const EGLint configAttribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_NONE
    };

    EGLint numConfigs;
    if (!eglChooseConfig(eglDisplay, configAttribs, &eglConfig, 1, &numConfigs)) {
        LOGE("Unable to choose EGL config");
        return false;
    }

    // 创建上下文
    const EGLint contextAttribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 3,
            EGL_NONE
    };
    eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, contextAttribs);
    if (eglContext == EGL_NO_CONTEXT) {
        LOGE("Unable to create EGL context");
        return false;
    }

    return true;
}

EGLSurface EGLCore::createWindowSurface(ANativeWindow *window) {
    if (window == nullptr) {
        LOGE("window is null");
        return EGL_NO_SURFACE;
    }

    EGLSurface surface = eglCreateWindowSurface(eglDisplay, eglConfig, window, NULL);
    if (surface == EGL_NO_SURFACE) {
        LOGE("Unable to create EGL surface");
    }
    return surface;
}

EGLSurface EGLCore::createOffscreenSurface(int width, int height) {
    const EGLint atrribs[] = {
            EGL_WIDTH, width,
            EGL_HEIGHT, height,
            EGL_NONE
    };

    EGLSurface surface = eglCreatePbufferSurface(eglDisplay, eglConfig, atrribs);
    if (surface == EGL_NO_SURFACE) {
        LOGE("Unable to create EGL offscreen surface");
    }
    return surface;
}

bool EGLCore::makeCurrent(EGLSurface surface) {
    return eglMakeCurrent(eglDisplay, surface, surface, eglContext);
}

bool EGLCore::swapBuffers(EGLSurface surface) {
    return eglSwapBuffers(eglDisplay, surface);
}

void EGLCore::destroySurface(EGLSurface surface) {
    if (surface != EGL_NO_SURFACE) {
        eglDestroySurface(eglDisplay, surface);
    }
}

void EGLCore::release() {
    if (eglDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (eglContext != EGL_NO_CONTEXT) {
            eglDestroyContext(eglDisplay, eglContext);
            eglContext = EGL_NO_CONTEXT;
        }
        eglTerminate(eglDisplay);
        eglDisplay = EGL_NO_DISPLAY;
    }
}

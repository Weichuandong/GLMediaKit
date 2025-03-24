//
// Created by Weichuandong on 2025/3/10.
//

#ifndef GLMEDIAKIT_EGLCORE_H
#define GLMEDIAKIT_EGLCORE_H

#include <EGL/egl.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <mutex>

#define LOG_TAG "EGLCore"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class EGLCore {
public:
    EGLCore();
    ~EGLCore();

    // EGL初始化
    bool init();

    // 创建窗口表面
    EGLSurface createWindowSurface(ANativeWindow* window);

    // 创建离屏表面（预留接口）
    EGLSurface createOffscreenSurface(int width, int height);

    // 使表面成为当前绘制目标
    bool makeCurrent();

    // 交换缓冲区
    bool swapBuffers();

    // 销毁表面
    void destroySurface();

    // 释放EGL上下文
    void release();

    // 获取当前EGL上下文
    EGLContext getContext() const { return eglContext; }

    // 获取Surface
    EGLSurface getSurface() const { return eglSurface; }
private:
    EGLDisplay eglDisplay;
    EGLContext eglContext;
    EGLConfig eglConfig;
    EGLSurface eglSurface;
    std::mutex surfaceMtx;

    ANativeWindow* mWindow;
};
#endif //GLMEDIAKIT_EGLCORE_H

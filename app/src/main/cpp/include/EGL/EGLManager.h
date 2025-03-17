//
// Created by Weichuandong on 2025/3/10.
//

#ifndef GLMEDIAKIT_EGLMANAGER_H
#define GLMEDIAKIT_EGLMANAGER_H

#include <android/native_window.h>
#include <jni.h>

#include "EGLCore.h"
#include "Renderer/IRenderer.h"
#include "RenderThread.h"
#include "Renderer/ImageRenderer.h"
#include "Renderer/GLRenderer.h"
#include "TextureManger.h"

#define LOG_TAG "EGLManager"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class RenderThread;

class EGLManager {
public:
    EGLManager();
    ~EGLManager();

    // 初始化EGL环境
    bool init();

    // 处理Surface生命周期
    void surfaceCreated(ANativeWindow* window);
    void surfaceChanged(int width, int height);
    void surfaceDestroyed();

    // 设置图像数据
    void setImage(void* data, int width, int height);

    // 预留方法：切换为OpenGL渲染
    // void useGLRenderer();

    bool checkAndResetSurfaceChanged(int& w, int& h);

    // 添加纹理
    int createTextureFromBitmap(JNIEnv* env, jobject bitmap, jstring key);

private:
    EGLCore* eglCore;
    EGLSurface eglSurface;
    IRenderer* renderer;
    RenderThread* renderThread;
    ANativeWindow* window;
    TextureManager* textureManager;

    int width;
    int height;

    // 添加以下成员变量来跟踪状态和尺寸变化
    bool surfaceSizeChanged;
    std::mutex stateMutex;
    // 释放资源
    void release();
};

#endif //GLMEDIAKIT_EGLMANAGER_H

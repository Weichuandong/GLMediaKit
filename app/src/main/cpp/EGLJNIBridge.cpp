//
// Created by Weichuandong on 2025/3/10.
//

#include <jni.h>
#include <android/native_window_jni.h>
#include "EGL/EGLManager.h"

extern "C"
JNIEXPORT jlong JNICALL
Java_com_example_glmediakit_EGLSurfaceView_nativeInit(JNIEnv *env, jobject thiz) {
    auto *manager = new EGLManager();
    if (!manager->init()) {
        delete manager;
        return 0;
    }
    return reinterpret_cast<jlong>(manager);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_glmediakit_EGLSurfaceView_nativeFinalize(JNIEnv *env, jobject thiz,
                                                             jlong handle) {
    if (handle != 0) {
        auto *manager = reinterpret_cast<EGLManager*>(handle);
        delete manager;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_glmediakit_EGLSurfaceView_nativeSurfaceCreated(JNIEnv *env, jobject thiz,
                                                                   jlong handle, jobject surface) {
    if (handle != 0) {
        auto *manager = reinterpret_cast<EGLManager*>(handle);
        auto *window = ANativeWindow_fromSurface(env, surface);
        manager->surfaceCreated(window);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_glmediakit_EGLSurfaceView_nativeSurfaceChanged(JNIEnv *env, jobject thiz,
                                                                   jlong handle, jint width,
                                                                   jint height) {
    if (handle != 0) {
        auto *manager = reinterpret_cast<EGLManager*>(handle);
        manager->surfaceChanged(width, height);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_glmediakit_EGLSurfaceView_nativeSurfaceDestroyed(JNIEnv *env, jobject thiz,
                                                                     jlong handle) {
    if (handle != 0) {
        auto *manager = reinterpret_cast<EGLManager*>(handle);
        manager->surfaceDestroyed();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_glmediakit_EGLSurfaceView_nativeSetImage(JNIEnv *env, jobject thiz,
                                                             jlong handle, jobject pixels,
                                                             jint width, jint height) {
    if (handle != 0) {
        auto *manager = reinterpret_cast<EGLManager*>(handle);
        void *data = env->GetDirectBufferAddress(pixels);
        manager->setImage(data, width, height);
    }
}
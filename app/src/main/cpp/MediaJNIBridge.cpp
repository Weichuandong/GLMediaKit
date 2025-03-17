//
// Created by Weichuandong on 2025/3/10.
//

#include <jni.h>
#include <android/native_window_jni.h>
#include "MediaManager.h"

extern "C"
JNIEXPORT jlong JNICALL
Java_com_example_glmediakit_MediaSurfaceView_nativeInit(JNIEnv *env, jobject thiz) {
    auto *manager = new MediaManager();
    if (!manager->init()) {
        delete manager;
        return 0;
    }
    return reinterpret_cast<jlong>(manager);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_glmediakit_MediaSurfaceView_nativeFinalize(JNIEnv *env, jobject thiz,
                                                             jlong handle) {
    if (handle != 0) {
        auto *manager = reinterpret_cast<MediaManager*>(handle);
        delete manager;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_glmediakit_MediaSurfaceView_nativeSurfaceCreated(JNIEnv *env, jobject thiz,
                                                                   jlong handle, jobject surface) {
    if (handle != 0) {
        auto *manager = reinterpret_cast<MediaManager*>(handle);
        auto *window = ANativeWindow_fromSurface(env, surface);
        manager->surfaceCreated(window);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_glmediakit_MediaSurfaceView_nativeSurfaceChanged(JNIEnv *env, jobject thiz,
                                                                   jlong handle, jint width,
                                                                   jint height) {
    if (handle != 0) {
        auto *manager = reinterpret_cast<MediaManager*>(handle);
        manager->surfaceChanged(width, height);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_glmediakit_MediaSurfaceView_nativeSurfaceDestroyed(JNIEnv *env, jobject thiz,
                                                                     jlong handle) {
    if (handle != 0) {
        auto *manager = reinterpret_cast<MediaManager*>(handle);
        manager->surfaceDestroyed();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_glmediakit_MediaSurfaceView_nativeSetImage(JNIEnv *env, jobject thiz,
                                                             jlong handle, jobject pixels,
                                                             jint width, jint height) {
    if (handle != 0) {
        auto *manager = reinterpret_cast<MediaManager*>(handle);
        void *data = env->GetDirectBufferAddress(pixels);
        manager->setImage(data, width, height);
    }
}
extern "C"
JNIEXPORT int JNICALL
Java_com_example_glmediakit_MediaSurfaceView_nativeCreateTexture(JNIEnv *env, jobject thiz,
                                                               jlong handle, jobject bitmap, jstring key) {
    if (handle == 0 || bitmap == NULL) {
        LOGE("Invalid handle or bitmap");
        return 0;
    }

    auto* manager = reinterpret_cast<MediaManager*>(handle);

    return manager->createTextureFromBitmap(env, bitmap, key);
}
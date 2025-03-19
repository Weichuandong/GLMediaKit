//
// Created by Weichuandong on 2025/3/19.
//
#include <jni.h>

#include "Player.h"

extern "C"
JNIEXPORT jlong JNICALL
Java_com_example_glmediakit_Player_nativeInit(JNIEnv *env, jobject thiz) {
    auto* player = new Player();
    if (!player->init()){
        delete player;
        return 0;
    }
    return reinterpret_cast<jlong>(player);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_glmediakit_Player_nativePrepare(JNIEnv *env, jobject thiz, jlong handle,
                                                 jstring file_path) {
    if (handle != 0) {
        auto* player = reinterpret_cast<Player*>(handle);

    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_glmediakit_Player_nativePlayback(JNIEnv *env, jobject thiz, jlong handle) {
    if (handle != 0) {
        auto* player = reinterpret_cast<Player*>(handle);
        player->playback();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_glmediakit_Player_nativePause(JNIEnv *env, jobject thiz, jlong handle) {
    if (handle != 0) {
        auto* player = reinterpret_cast<Player*>(handle);
        player->pause();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_glmediakit_Player_nativeResume(JNIEnv *env, jobject thiz, jlong handle) {
    if (handle != 0) {
        auto* player = reinterpret_cast<Player*>(handle);

    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_glmediakit_Player_nativeStop(JNIEnv *env, jobject thiz, jlong handle) {
    if (handle != 0) {
        auto* player = reinterpret_cast<Player*>(handle);
        player->stop();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_glmediakit_Player_nativeSurfaceCreate(JNIEnv *env, jobject thiz, jlong handle,
                                                       jobject surface) {
    if (handle != 0) {
        auto* player = reinterpret_cast<Player*>(handle);
        auto* window = ANativeWindow_fromSurface(env, surface);
        player->attachSurface(window);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_glmediakit_Player_nativeSurfaceChanged(JNIEnv *env, jobject thiz, jlong handle,
                                                        jint width, jint heigth) {
    if (handle != 0) {
        auto* player = reinterpret_cast<Player*>(handle);
        player->surfaceSizeChanged(width, heigth);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_glmediakit_Player_nativeSurfaceDestroyed(JNIEnv *env, jobject thiz, jlong handle) {
    if (handle != 0) {
        auto* player = reinterpret_cast<Player*>(handle);
        player->detachSurface();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_glmediakit_Player_nativeRelease(JNIEnv *env, jobject thiz, jlong handle) {
    if (handle != 0) {
        auto* player = reinterpret_cast<Player*>(handle);
        delete player;
    }
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_glmediakit_Player_nativeGetPlayerState(JNIEnv *env, jobject thiz, jlong handle) {
    if (handle != 0) {
        auto* player = reinterpret_cast<Player*>(handle);
        return static_cast<jint>(player->getPlayerState());
    }
}

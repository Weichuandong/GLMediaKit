//
// Created by Weichuandong on 2025/4/2.
//

#ifndef GLMEDIAKIT_MEDIACODECDECODERWRAPPER_H
#define GLMEDIAKIT_MEDIACODECDECODERWRAPPER_H

#include <jni.h>
#include <string>
#include <android/log.h>

#include "JNIHelper.h"

#define LOG_TAG "MediaCodecDecoderWrapper"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

class MediaCodecDecoderWrapper {
public:
    MediaCodecDecoderWrapper();
    ~MediaCodecDecoderWrapper();

    bool init(const std::string& mimeType, int width, int height,
              const uint8_t* sps, int spsSize, const uint8_t* pps, int ppsSize,
              bool surface = false);

    bool pushEncodedData(const uint8_t* data, size_t size ,int64_t pts, int flag);

    bool getDecodedData(uint8_t* outBuffer, size_t* outSize, int64_t* outPts);

private:
    // JNI相关
    JavaVM* javaVM;
    jobject  decoderObject;
    jmethodID initMethod;
    jmethodID queueInputBufferMethod;
    jmethodID dequeOutputBufferMethod;
    jmethodID getOutputBufferMethod;
    jmethodID signalEndOfInputStreamMethod;
    jmethodID releaseMethod;

    bool surface;

    // 初始化JNI环境
    bool initJNI();
    JNIEnv* getEnv();
};

#endif //GLMEDIAKIT_MEDIACODECDECODERWRAPPER_H

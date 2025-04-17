//
// Created by Weichuandong on 2025/4/2.
//

#ifndef GLMEDIAKIT_MEDIACODECDECODERWRAPPER_H
#define GLMEDIAKIT_MEDIACODECDECODERWRAPPER_H

#include <jni.h>
#include <string>
#include <android/log.h>
#include <vector>

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

    bool pushEncodedData(const uint8_t* data, int size, int ts, int flag);

    bool getDecodedData(std::vector<uint8_t>& outBuffer, size_t& outSize, int64_t& outPts, int& bufferId);

    bool releaseOutputBuffer(int outputBufferId);
private:
    // JNI相关
    JavaVM* javaVM;
    jobject  decoderObject;
    // MediaCodecDecoder相关方法
    jmethodID initMethod;
    jmethodID pushInputBufferMethod;
    jmethodID getOutputBufferMethod;
    jmethodID releaseOutputBufferMethod;
    jmethodID getPtsMethod;
    jmethodID signalEndOfInputStreamMethod;
    jmethodID releaseMethod;
    // DecodedFrame相关方法
    jmethodID getBufferMethod;
    jmethodID getOutputBufferIdMethod;

    bool havaSurface;

    // 初始化JNI环境
    bool initJNI();
    JNIEnv* getEnv();
};

#endif //GLMEDIAKIT_MEDIACODECDECODERWRAPPER_H

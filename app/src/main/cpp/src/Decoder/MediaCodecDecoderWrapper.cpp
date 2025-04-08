//
// Created by Weichuandong on 2025/4/3.
//

#include "Decoder/MediaCodecDecoderWrapper.h"


MediaCodecDecoderWrapper::MediaCodecDecoderWrapper() :
    javaVM(nullptr),
    decoderObject(nullptr),
    surface(false)
{
    initJNI();
}

MediaCodecDecoderWrapper::~MediaCodecDecoderWrapper() {
    if (decoderObject != nullptr) {
        JNIEnv* env = getEnv();
        if (env && releaseMethod) {
            env->CallVoidMethod(decoderObject, releaseMethod);
        }
        env->DeleteGlobalRef(decoderObject);
        decoderObject = nullptr;
    }
}

bool MediaCodecDecoderWrapper::init(const std::string& mimeType, int width, int height,
                                    const uint8_t *sps, int spsSize, const uint8_t *pps,
                                    int ppsSize, bool surface) {
    return true;
}

bool MediaCodecDecoderWrapper::pushEncodedData(const uint8_t *data, size_t size, int64_t pts, int flag) {


    return true;
}

bool MediaCodecDecoderWrapper::getDecodedData(uint8_t *outBuffer, size_t *outSize, int64_t *outPts) {


    return true;
}

bool MediaCodecDecoderWrapper::initJNI() {
    // 获取JavaVM
//    if (javaVM == nullptr) {
//        jint result = JNI_GetCreatedJavaVMs(&javaVM, 1, nullptr);
//        if (result != JNI_OK || javaVM == nullptr) {
//            LOGE("Failed to get javaVM");
//            return false;
//        }
//    }
    if (g_jvm == nullptr) {
        LOGE("JavaVM is null, JNI_OnLoad might not have been called");
        return false;
    }
    javaVM = g_jvm;

    // 获取JNIEnv
    JNIEnv* env = nullptr;
    jint attachResult = javaVM->AttachCurrentThread(&env, nullptr);
    if (attachResult != JNI_OK || env == nullptr) {
        LOGE("Failed to attach current thread");
        return false;
    }

    // 查找MediaCodecDecoder类
    jclass localDecoderRef = env->FindClass("com/example/glmediakit/decoder/MediaCodecDecoder");
    if (localDecoderRef == nullptr) {
        LOGE("Failed to find MediaCodecVideoDecoder class");
        return false;
    }

    // 创建全局应用
    jclass decoderClass = (jclass)env->NewGlobalRef(localDecoderRef);
    env->DeleteLocalRef(localDecoderRef);

    // 获取构造方法
    jmethodID constructor = env->GetMethodID(decoderClass, "<init>", "()V");
    if (constructor == nullptr) {
        LOGE("Failed to find constructor");
        env->DeleteGlobalRef(decoderClass);
        return false;
    }

    // 创建MediaCodecDecoder对象
    jobject localObject = env->NewObject(decoderClass, constructor);
    if (localObject == nullptr) {
        LOGE("Failed to create decoder object");
        env->DeleteGlobalRef(decoderClass);
        return false;
    }

    decoderObject = env->NewGlobalRef(localObject);
    env->DeleteLocalRef(localObject);

    // 获取方法ID
    initMethod = env->GetMethodID(decoderClass, "initialize",
                                  "(Ljava/lang/String;II[Ljava/nio/ByteBuffer;Z)Z");
    queueInputBufferMethod = env->GetMethodID(decoderClass, "queueInputBuffer",
                                             "(Ljava/nio/ByteBuffer;IJI)Z");
    dequeOutputBufferMethod = env->GetMethodID(decoderClass, "dequeOutputBuffer",
                                               "(J)I");
    getOutputBufferMethod = env->GetMethodID(decoderClass, "getOutputBuffer",
                                             "(I)Ljava/nio/ByteBuffer;");
    signalEndOfInputStreamMethod = env->GetMethodID(decoderClass, "signalEndOfInputStream",
                                                    "()V");
    releaseMethod = env->GetMethodID(decoderClass, "release",
                                     "()V");

    env->DeleteGlobalRef(decoderClass);
    return initMethod && queueInputBufferMethod && dequeOutputBufferMethod &&
            getOutputBufferMethod && signalEndOfInputStreamMethod && releaseMethod;
}

JNIEnv *MediaCodecDecoderWrapper::getEnv() {
    JNIEnv* env = nullptr;
    if (javaVM == nullptr) {
        return nullptr;
    }
    jint result = javaVM->GetEnv((void **)&env, JNI_VERSION_1_6);
    if (result == JNI_EDETACHED) {
        javaVM->AttachCurrentThread(&env, nullptr);
    }


    return env;
}

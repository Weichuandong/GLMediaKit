//
// Created by Weichuandong on 2025/4/3.
//

#include "Decoder/MediaCodecDecoderWrapper.h"


MediaCodecDecoderWrapper::MediaCodecDecoderWrapper() :
    javaVM(nullptr),
    decoderObject(nullptr),
    havaSurface(false)
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
    if (!decoderObject) {
        LOGE("decoderObject is null, maybe initJNI failed");
        return false;
    }
    havaSurface = surface;

    // 构造Java层解码器初始化参数
    JNIEnv* env = getEnv();
    if (env && initMethod) {
        jstring jMimeType = env->NewStringUTF(mimeType.c_str());

        // 创建ByteBuffer数组
        jclass byteBufferClass = env->FindClass("java/nio/ByteBuffer");

        // 创建长度为2的ByteBuffer数组（SPS，PPS）
        jobjectArray csdsArray = env->NewObjectArray(2, byteBufferClass, nullptr);

        // 添加SPS
        if (sps && spsSize > 0) {
            jobject spsByteBuffer = env->CallStaticObjectMethod(
                    byteBufferClass,
                    env->GetStaticMethodID(byteBufferClass, "allocateDirect", "(I)Ljava/nio/ByteBuffer;"),
                    spsSize
                    );
            auto* directBufferAddress = static_cast<uint8_t*>(env->GetDirectBufferAddress(spsByteBuffer));
            if (directBufferAddress) {
                memcpy(directBufferAddress, sps, spsSize);
            }
            // 将position 重置为0
            env->CallObjectMethod(
                    spsByteBuffer,
                    env->GetMethodID(byteBufferClass, "position", "(I)Ljava/nio/Buffer;"),
                    0
            );
            env->SetObjectArrayElement(csdsArray, 0, spsByteBuffer);
            env->DeleteLocalRef(spsByteBuffer);
        }
        // 添加PPS
        if (pps && ppsSize > 0) {
            jobject ppsByteBuffer = env->CallStaticObjectMethod(
                    byteBufferClass,
                    env->GetStaticMethodID(byteBufferClass, "allocateDirect", "(I)Ljava/nio/ByteBuffer;"),
                    ppsSize
                    );
            auto* directBufferAddress = static_cast<uint8_t*>(env->GetDirectBufferAddress(ppsByteBuffer));
            if (directBufferAddress) {
                memcpy(directBufferAddress, pps, ppsSize);
            }
            // 将position 重置为0
            env->CallObjectMethod(
                    ppsByteBuffer,
                    env->GetMethodID(byteBufferClass, "position", "(I)Ljava/nio/Buffer;"),
                    0
            );
            env->SetObjectArrayElement(csdsArray, 1, ppsByteBuffer);
            env->DeleteLocalRef(ppsByteBuffer);
        }

        // 调用构造方法
        jboolean rst = env->CallBooleanMethod(decoderObject, initMethod,
                                              jMimeType, width, height, csdsArray, surface);

        // 清理引用
        env->DeleteLocalRef(jMimeType);
        env->DeleteLocalRef(csdsArray);
        env->DeleteLocalRef(byteBufferClass);

        return (bool)rst;
    }

    return false;
}

bool MediaCodecDecoderWrapper::pushEncodedData(const uint8_t *data, int size, int ts, int flag) {
    if (!decoderObject) {
        LOGE("decoderObject is null, maybe initJNI failed");
        return false;
    }
    JNIEnv* env = getEnv();
    if (env && pushInputBufferMethod) {
        jclass byteBufferClass = env->FindClass("java/nio/ByteBuffer");

        // 创建ByteBuffer
        jobject dataBuffer = env->CallStaticObjectMethod(
                byteBufferClass,
                env->GetStaticMethodID(byteBufferClass, "allocateDirect", "(I)Ljava/nio/ByteBuffer;"),
                size
                );
        // 复制数据到ByteBuffer
        auto* directBufferAddress = static_cast<uint8_t*>(env->GetDirectBufferAddress(dataBuffer));
        if (directBufferAddress) {
            memcpy(directBufferAddress, data, size);
        }

        // 调用Java方法
        jboolean rst = env->CallBooleanMethod(decoderObject, pushInputBufferMethod, dataBuffer, size, ts, flag);

        // 清理引用
        env->DeleteLocalRef(dataBuffer);
        env->DeleteLocalRef(byteBufferClass);

        return (bool)rst;
    }

    return false;
}

bool MediaCodecDecoderWrapper::getDecodedData(std::vector<uint8_t>& outBuffer, size_t& outSize, int64_t& outPts, int& bufferId) {
    if (!decoderObject) {
        LOGE("decoderObject is null, maybe initJNI failed");
        return false;
    }
    JNIEnv* env = getEnv();
    if (env && getOutputBufferMethod) {
        // 调用Java方法
        jobject decodedFrameObject = env->CallObjectMethod(decoderObject, getOutputBufferMethod, (jlong)10000);

        if (!decodedFrameObject) {
            // 没有可用输出数据
            return false;
        }

        // 获取Buffer信息
        jobject dataBuffer = env->CallObjectMethod(decodedFrameObject, getBufferMethod);
        jclass bufferClass = env->GetObjectClass(dataBuffer);

        // 获取缓冲区大小
        jmethodID remainingMethod = env->GetMethodID(bufferClass, "remaining", "()I");
        jint size = env->CallIntMethod(dataBuffer, remainingMethod);
        if (size <= 0) {
            LOGE("dataBuffer size <= 0");
            return false;
        }
        outSize = size;

        // 获取缓冲区中的数据
        auto* directBuffer = static_cast<uint8_t*>(env->GetDirectBufferAddress(dataBuffer));
        if (directBuffer) {
            outBuffer.reserve(size);
            outBuffer.assign(directBuffer, directBuffer + size);
        } else {
            LOGE("directBuffer is null");
            return false;
        }

        // 获取时间戳
        outPts = env->CallLongMethod(decoderObject, getPtsMethod);

        // 获取bufferId
        bufferId = env->CallIntMethod(decodedFrameObject, getOutputBufferIdMethod);

        return true;
    }

    return true;
}

bool MediaCodecDecoderWrapper::releaseOutputBuffer(int outputBufferId) {
    if (!decoderObject) {
        LOGE("decoderObject is null, maybe initJNI failed");
        return false;
    }
    JNIEnv *env = getEnv();
    LOGI("MediaCodecDecoderWrapper::getDecodedData start");
    if (env && releaseOutputBufferMethod) {
        // 调用方法通知

        return env->CallBooleanMethod(decoderObject, releaseOutputBufferMethod, outputBufferId);
    }
    return false;
}

bool MediaCodecDecoderWrapper::initJNI() {
    // 获取JavaVM
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

    // 获取构造方法
    jmethodID constructor = env->GetMethodID(localDecoderRef, "<init>", "()V");
    if (constructor == nullptr) {
        LOGE("Failed to find constructor");
        env->DeleteGlobalRef(localDecoderRef);
        return false;
    }

    // 创建MediaCodecDecoder对象
    jobject localObject = env->NewObject(localDecoderRef, constructor);
    if (localObject == nullptr) {
        LOGE("Failed to create decoder object");
        env->DeleteGlobalRef(localDecoderRef);
        return false;
    }

    decoderObject = env->NewGlobalRef(localObject);
    env->DeleteLocalRef(localObject);

    // 获取方法ID
    initMethod = env->GetMethodID(localDecoderRef, "initialize",
                                  "(Ljava/lang/String;II[Ljava/nio/ByteBuffer;Z)Z");
    pushInputBufferMethod = env->GetMethodID(localDecoderRef, "pushInputBuffer",
                                             "(Ljava/nio/ByteBuffer;III)Z");
    getOutputBufferMethod = env->GetMethodID(localDecoderRef, "getOutputBuffer",
                                             "(J)Lcom/example/glmediakit/decoder/DecodedFrame;");
    releaseOutputBufferMethod = env->GetMethodID(localDecoderRef, "releaseOutputBuffer",
                                                 "(I)Z");
    getPtsMethod = env->GetMethodID(localDecoderRef, "getPts",
                                    "()J");
    signalEndOfInputStreamMethod = env->GetMethodID(localDecoderRef, "signalEndOfInputStream",
                                                    "()V");
    releaseMethod = env->GetMethodID(localDecoderRef, "release",
                                     "()V");

    // 查找DecodedFrame类
    jclass localFrameRef = env->FindClass("com/example/glmediakit/decoder/DecodedFrame");
    if (localFrameRef == nullptr) {
        LOGE("Failed to find DecodedFrame class");
        return false;
    }

    getBufferMethod = env->GetMethodID(localFrameRef, "getBuffer", "()Ljava/nio/ByteBuffer;");
    getOutputBufferIdMethod = env->GetMethodID(localFrameRef, "getOutputBufferId", "()I");


    return initMethod && pushInputBufferMethod && getOutputBufferMethod &&
           signalEndOfInputStreamMethod && releaseMethod && getBufferMethod && getOutputBufferIdMethod;
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

//
// Created by Weichuandong on 2025/3/25.
//
#include "Decoder/FFMpegAudioDecoder.h"

FFMpegAudioDecoder::FFMpegAudioDecoder() {

}

FFMpegAudioDecoder::~FFMpegAudioDecoder() {
    release();
}

bool FFMpegAudioDecoder::configure(const AVCodecParameters *codecParams) {
    if (!codecParams) {
        LOGE("Invalid codec parameters");
        return false;
    }

    // 查找解码器
    const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
    if (!codec) {
        LOGE("Could not find decoder : %d", codecParams->codec_id);
        return false;
    }

    // 分配解码器上下文
    avCodecContext = avcodec_alloc_context3(codec);
    if (!avCodecContext) {
        LOGE("Could not alloc codecContext");
        return false;
    }
    // 将参数复制到解码器上下文
    if (avcodec_parameters_to_context(avCodecContext, codecParams) < 0) {
        LOGE("Could not parameters to codecContext");
        return false;
    }

    // 打开解码器
    if (avcodec_open2(avCodecContext, codec, nullptr) < 0) {
        LOGE("Could not open decoder");
        return -1;
    }

    inChannel = avCodecContext->channels;
    inSampleFormat = avCodecContext->sample_fmt;
    inSampleRate = avCodecContext->sample_rate;

    isReady = true;

    LOGI("Audio decoder configured: channel = %d, sampleRate = %d, sampleFormat = %d", inChannel, inSampleRate, inSampleFormat);
    return true;
}

int FFMpegAudioDecoder::SendPacket(const AVPacket *packet) {
    // 发送包到解码器
    int sendResult = avcodec_send_packet(avCodecContext, packet);
    if (sendResult < 0) {
        char errString[128];
        av_strerror(sendResult, errString, 128);
        LOGE("avcodec_send_packet failed due to '%s'", errString);
    }

    return sendResult;
}

int FFMpegAudioDecoder::ReceiveFrame(AVFrame *frame) {
    int ret = avcodec_receive_frame(avCodecContext, frame);

    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        // 不是错误
    } else if (ret < 0) {
        char errString[128];
        av_strerror(ret, errString, 128);
        LOGE("avcodec_receive_frame failed due to '%s'", errString);
    }
    return ret;
}

void FFMpegAudioDecoder::release() {
    if (avCodecContext) {
        avcodec_flush_buffers(avCodecContext);
        avcodec_free_context(&avCodecContext);
    }
    isReady = false;
}
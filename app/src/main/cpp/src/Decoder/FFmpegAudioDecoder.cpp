//
// Created by Weichuandong on 2025/3/25.
//
#include "Decoder/FFmpegAudioDecoder.h"

FFmpegAudioDecoder::FFmpegAudioDecoder() {

}

FFmpegAudioDecoder::~FFmpegAudioDecoder() {
    release();
}

bool FFmpegAudioDecoder::configure(const DecoderConfig& codecParams) {
//    if (!codecParams) {
//        LOGE("Invalid codec parameters");
//        return false;
//    }
    auto param = codecParams.param;
    // 查找解码器
    const AVCodec* codec = avcodec_find_decoder(param->codec_id);
    if (!codec) {
        LOGE("Could not find decoder : %d", param->codec_id);
        return false;
    }

    // 分配解码器上下文
    avCodecContext = avcodec_alloc_context3(codec);
    if (!avCodecContext) {
        LOGE("Could not alloc codecContext");
        return false;
    }
    // 将参数复制到解码器上下文
    if (avcodec_parameters_to_context(avCodecContext, param) < 0) {
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

int FFmpegAudioDecoder::SendPacket(const std::shared_ptr<IMediaPacket>& packet) {
    // 发送包到解码器'
    auto mediaPacket = packet->asAVPacket();
    int sendResult = avcodec_send_packet(avCodecContext, mediaPacket);
    if (sendResult < 0) {
        char errString[128];
        av_strerror(sendResult, errString, 128);
        LOGE("avcodec_send_packet failed due to '%s'", errString);
    }

    return sendResult;
}

int FFmpegAudioDecoder::ReceiveFrame(std::shared_ptr<IMediaFrame>& frame) {
    auto mediaFrame = frame->asAVFrame();
    int ret = avcodec_receive_frame(avCodecContext, mediaFrame);

    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        // 不是错误
    } else if (ret < 0) {
        char errString[128];
        av_strerror(ret, errString, 128);
        LOGE("avcodec_receive_frame failed due to '%s'", errString);
    }
    return ret;
}

void FFmpegAudioDecoder::release() {
    if (avCodecContext) {
        avcodec_flush_buffers(avCodecContext);
        avcodec_free_context(&avCodecContext);
    }
    isReady = false;
}

SampleFormat FFmpegAudioDecoder::getSampleFormat() {
    switch (avCodecContext->sample_fmt) {
        case AV_SAMPLE_FMT_S16:
            return SampleFormat::S16;
        case AV_SAMPLE_FMT_S16P:
            return SampleFormat::S16P;
        default:
            return SampleFormat::UNKNOWN;
    }
}

//
// Created by Weichuandong on 2025/3/17.
//

#include "Decoder/FFmpegVideoDecoder.h"

FFmpegVideoDecoder::FFmpegVideoDecoder() :
    avCodecContext(nullptr),
    mWidth(0),
    mHeight(0),
    format(AV_PIX_FMT_NONE),
    isReady(false)
{

}

FFmpegVideoDecoder::~FFmpegVideoDecoder() {
    release();
}

bool FFmpegVideoDecoder::configure(const DecoderConfig& codecParams) {
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
    avCodecContext->thread_count = 8;

    // 打开解码器
    if (avcodec_open2(avCodecContext, codec, nullptr) < 0) {
        LOGE("Could not open decoder");
        return -1;
    }

    // 保存视频属性
    mWidth = avCodecContext->width;
    mHeight = avCodecContext->height;
    format = avCodecContext->pix_fmt;
    isReady = true;

    LOGI("Video decoder configured: %dx%d, pixel format: %d", mWidth, mHeight, format);
    return true;
}

int FFmpegVideoDecoder::SendPacket(const std::shared_ptr<IMediaPacket>& packet) {
    // 发送包到解码器
    auto avPacket = packet->asAVPacket();
    int sendResult = avcodec_send_packet(avCodecContext, avPacket);
    if (sendResult < 0) {
        char errString[128];
        av_strerror(sendResult, errString, 128);
        LOGE("avcodec_send_packet failed due to '%s'", errString);
    }

    return sendResult;
}

int FFmpegVideoDecoder::ReceiveFrame(std::shared_ptr<IMediaFrame>& frame) {
    auto avFrame = frame->asAVFrame();

    int ret = avcodec_receive_frame(avCodecContext, avFrame);

    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {

    } else if (ret < 0) {
        char errString[128];
        av_strerror(ret, errString, 128);
        LOGE("avcodec_receive_frame failed due to '%s'", errString);
    }
    return ret;
}

void FFmpegVideoDecoder::release() {
    if (avCodecContext) {
        avcodec_flush_buffers(avCodecContext);
        avcodec_free_context(&avCodecContext);
    }
    isReady = false;
}

PixFormat FFmpegVideoDecoder::getPixFormat() {
    switch (avCodecContext->pix_fmt) {
        case AV_PIX_FMT_YUV420P:
            return PixFormat::YUV420P;
        case AV_PIX_FMT_RGB24:
            return PixFormat::RGBA24;
        case AV_PIX_FMT_NV12:
            return PixFormat::NV12;
        default:
            return PixFormat::UNKNOWN;
    }
}

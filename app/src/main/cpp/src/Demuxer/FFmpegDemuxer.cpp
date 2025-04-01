//
// Created by Weichuandong on 2025/3/20.
//

#include "Demuxer/FFmpegDemuxer.h"

#include <utility>

FFmpegDemuxer::FFmpegDemuxer(FFmpegDemuxer::DemuxerType type) :
        fmt_ctx(nullptr),
        streamIdx(-1),
        type(type),
        duration(0)
{

}

FFmpegDemuxer::~FFmpegDemuxer() {

}

bool FFmpegDemuxer::open(const std::string &file_path) {
    filePath = file_path;

    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
        fmt_ctx = nullptr;
    }

    // 打开文件
    int ret = avformat_open_input(&fmt_ctx, file_path.c_str(), nullptr, nullptr);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
        LOGE("Could not open input : %s", errbuf);
        return false;
    }

    // 获取流信息
    ret = avformat_find_stream_info(fmt_ctx, nullptr);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
        LOGE("Could not find stream info : %s", errbuf);
        avformat_close_input(&fmt_ctx);
        fmt_ctx = nullptr;
        return false;
    }

    // 寻找音频 或 视频流
    for (int i = 0; i < fmt_ctx->nb_streams; ++i) {
        AVStream* stream = fmt_ctx->streams[i];
        if (type == DemuxerType::AUDIO && stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            LOGI("Audio streamIdx = %d", i);
            streamIdx = i;
            break;
        } else if ( type == DemuxerType::VIDEO && stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            LOGI("Video streamIdx = %d", i);
            streamIdx = i;
            break;
        }
    }

    // 获取媒体时长
    if (fmt_ctx->duration != AV_NOPTS_VALUE) {
        duration = fmt_ctx->duration / (double)AV_TIME_BASE;
    }

    // 打印一些调试信息
    LOGI("Media opened: %s", filePath.c_str());
    LOGI("Duration: %.2f seconds", duration);
    if (type == DemuxerType::AUDIO) LOGI("Has audio: %s", hasAudio() ? "yes" : "no");
    if (type == DemuxerType::VIDEO) LOGI("Has video: %s", hasVideo() ? "yes" : "no");

    return streamIdx >= 0;
}

void FFmpegDemuxer::seekTo(double position) {
    isSeekRequested = true;
    seekPosition = position;
}

AVCodecParameters* FFmpegDemuxer::getCodecParameters() {
    if (!fmt_ctx || streamIdx < 0) return nullptr;
    return fmt_ctx->streams[streamIdx]->codecpar;
}

bool FFmpegDemuxer::hasVideo() const {
    return type == DemuxerType::VIDEO && streamIdx >= 0;
}

bool FFmpegDemuxer::hasAudio() const {
    return type == DemuxerType::AUDIO && streamIdx >= 0;
}

AVRational FFmpegDemuxer::getTimeBase() const {
    if (streamIdx >= 0 && fmt_ctx && fmt_ctx->streams[streamIdx]) {
        LOGI("TimeBase = {%d, %d}",
             fmt_ctx->streams[streamIdx]->time_base.num,
             fmt_ctx->streams[streamIdx]->time_base.den);
        return fmt_ctx->streams[streamIdx]->time_base;
    }
    return AVRational{0, 0};
}

int FFmpegDemuxer::ReceivePacket(AVPacket *packet) {
    // 获取Packet, 并且返回类型
    int ret;
    while (true) {
        ret = av_read_frame(fmt_ctx, packet);
        if (ret >= 0) {
            if (packet->stream_index == streamIdx) {
                // 获取对应流packet
                break;
            } else {
                av_packet_unref(packet);
                continue;
            }
        } else if (ret == AVERROR_EOF) {

        } else {
            char errString[128];
            av_strerror(ret, errString, 128);
            LOGE("av_read_frame failed due to '%s'", errString);
        }
    }

    return ret;
}

void FFmpegDemuxer::release() {
    if (fmt_ctx) {
        avformat_free_context(fmt_ctx);
    }
}
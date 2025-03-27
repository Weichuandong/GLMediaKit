//
// Created by Weichuandong on 2025/3/20.
//

#include "Demuxer/FFmpegDemuxer.h"

#include <utility>

FFmpegDemuxer::FFmpegDemuxer() :
        fmt_ctx(nullptr),
        videoStreamIdx(-1),
        audioStreamIdx(-1),
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

    // 寻找音频和视频流
    for (int i = 0; i < fmt_ctx->nb_streams; ++i) {
        AVStream* stream = fmt_ctx->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && videoStreamIdx < 0) {
            videoStreamIdx = i;
        } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audioStreamIdx < 0) {
            audioStreamIdx = i;
        }
    }

    // 获取媒体时长
    if (fmt_ctx->duration != AV_NOPTS_VALUE) {
        duration = fmt_ctx->duration / (double)AV_TIME_BASE;
    }

    // 打印一些调试信息
    LOGI("Media opened: %s", filePath.c_str());
    LOGI("Duration: %.2f seconds", duration);
    LOGI("Has video: %s", hasVideo() ? "yes" : "no");
    LOGI("Has audio: %s", hasAudio() ? "yes" : "no");

    return videoStreamIdx >= 0 || audioStreamIdx > 0;
}

void FFmpegDemuxer::seekTo(double position) {
    isSeekRequested = true;
    seekPosition = position;
}

AVCodecParameters* FFmpegDemuxer::getVideoCodecParameters() {
    if (!fmt_ctx || videoStreamIdx < 0) return nullptr;
    return fmt_ctx->streams[videoStreamIdx]->codecpar;
}

AVCodecParameters *FFmpegDemuxer::getAudioCodecParameters() {
    if (!fmt_ctx || videoStreamIdx < 0) return nullptr;
    return fmt_ctx->streams[audioStreamIdx]->codecpar;
}

bool FFmpegDemuxer::hasVideo() const {
    return videoStreamIdx >= 0;
}

bool FFmpegDemuxer::hasAudio() const {
    return audioStreamIdx >= 0;
}

AVRational FFmpegDemuxer::getAudioTimeBase() const {
    if (audioStreamIdx >= 0 && fmt_ctx && fmt_ctx->streams[audioStreamIdx]) {
        LOGI("Audio timeBase = {%d, %d}",
             fmt_ctx->streams[audioStreamIdx]->time_base.num,
             fmt_ctx->streams[audioStreamIdx]->time_base.den);
        return fmt_ctx->streams[audioStreamIdx]->time_base;
    }
    return AVRational{0, 0};
}

AVRational FFmpegDemuxer::getVideoTimeBase() const {
    if (videoStreamIdx >= 0 && fmt_ctx && fmt_ctx->streams[videoStreamIdx]) {
        LOGI("Video timeBase = {%d, %d}",
             fmt_ctx->streams[videoStreamIdx]->time_base.num,
             fmt_ctx->streams[videoStreamIdx]->time_base.den);
        return fmt_ctx->streams[videoStreamIdx]->time_base;
    }
    return AVRational{0, 0};
}

FFmpegDemuxer::PacketType FFmpegDemuxer::ReceivePacket(AVPacket *packet) {
    // 获取Packet, 并且返回类型
    int ret = av_read_frame(fmt_ctx, packet);
    if (ret < 0) {
        char errString[128];
        av_strerror(ret, errString, 128);
        LOGE("av_read_frame failed due to '%s'", errString);
        return PacketType::NONE;
    }

    if (packet->stream_index == audioStreamIdx) {
        return PacketType::AUDIO;
    } else if (packet->stream_index == videoStreamIdx) {
        return PacketType::VIDEO;
    }

    return PacketType::NONE;
}

void FFmpegDemuxer::release() {
    if (fmt_ctx) {
        avformat_free_context(fmt_ctx);
    }
}
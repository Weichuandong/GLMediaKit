//
// Created by Weichuandong on 2025/3/20.
//

#include "Demuxer/FFmpegDemuxer.h"

#include <utility>

FFmpegDemuxer::FFmpegDemuxer(std::shared_ptr<SafeQueue<AVPacket *>> videoPacketQueue,
                             std::shared_ptr<SafeQueue<AVPacket *>> audioPacketQueue) :
        videoPacketQueue(std::move(videoPacketQueue)),
        audioPacketQueue(std::move(audioPacketQueue)),
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

void FFmpegDemuxer::start() {
    if (!fmt_ctx) return;

    isPaused = false;
    isEOF = false;
    exitRequested = false;
    isSeekRequested = false;
    isReady = true;

    // 启动线程
    demuxingThread = std::thread(&FFmpegDemuxer::demuxingThreadFunc, this);
}

void FFmpegDemuxer::pause() {
    isPaused = true;
}

void FFmpegDemuxer::resume() {
    isPaused = false;
    pauseCond.notify_one();
}

void FFmpegDemuxer::stop() {
    exitRequested = true;
    resume();

    if (demuxingThread.joinable()) {
        demuxingThread.join();
    }
}

void FFmpegDemuxer::seekTo(double position) {
    {
        std::unique_lock<std::mutex> lock(mtx);
        isSeekRequested = true;
        seekPosition = position;
    }
    resume();
}

AVCodecParameters* FFmpegDemuxer::getVideoCodecParameters() {
    if (!fmt_ctx || videoStreamIdx < 0) return nullptr;
    return fmt_ctx->streams[videoStreamIdx]->codecpar;
}

AVCodecParameters *FFmpegDemuxer::getAudioCodecParameters() {
    if (!fmt_ctx || videoStreamIdx < 0) return nullptr;
    return fmt_ctx->streams[audioStreamIdx]->codecpar;
}

void FFmpegDemuxer::demuxingThreadFunc() {
    if (!fmt_ctx) return;

    LOGI("FFmpegDemuxer : start demuxing thread");

    AVPacket packet;
    auto lastLogTime = std::chrono::steady_clock::now();
    // 数据
    uint16_t videoPacketCount{0};
    uint16_t audioPacketCount{0};

    while (!exitRequested) {
//        PerformanceTimer timer("Demux");
        {
            std::unique_lock<std::mutex> lock(mtx);
            while (isPaused && !exitRequested && !isSeekRequested) {
                pauseCond.wait(lock);
            }
        }

        // 处理Seek
        if (isSeekRequested && !exitRequested) {
            int64_t seekTarget = static_cast<int64_t>(seekPosition * AV_TIME_BASE);

            int flags = AVSEEK_FLAG_BACKWARD;
            int ret = -1;

            // 使用视频流
            if (videoStreamIdx >= 0) {
                AVStream* stream = fmt_ctx->streams[videoStreamIdx];
                int64_t ts = av_rescale_q(seekTarget, AV_TIME_BASE_Q, stream->time_base);
                ret = av_seek_frame(fmt_ctx, videoStreamIdx, ts, flags);
            }

            // 如果视频Seek失败，尝试音频
            if (ret < 0 && audioStreamIdx >= 0) {
                AVStream* stream = fmt_ctx->streams[audioStreamIdx];
                int64_t ts = av_rescale_q(seekTarget, AV_TIME_BASE_Q, stream->time_base);
                ret = av_seek_frame(fmt_ctx, audioStreamIdx, ts, flags);
            }

            // 最后尝试默认流
            if (ret < 0) {
                ret = av_seek_frame(fmt_ctx, -1, seekTarget, flags);
            }

            // 清除缓冲区
            // 创建一个特殊包表示需要清空解码器缓冲区
            if (ret >= 0) {
                isEOF = false;
                if (videoPacketQueue && hasVideo()) {
                    AVPacket *flushPkt = av_packet_alloc();
                    av_packet_make_refcounted(flushPkt);
                    flushPkt->data = nullptr;
                    flushPkt->size = 0;
                    flushPkt->stream_index = videoStreamIdx;
                    flushPkt->flags = 0xFFFF;
                    videoPacketQueue->push(flushPkt, 10);
                }
                if (audioPacketQueue && hasAudio()) {
                    AVPacket *flushPkt = av_packet_alloc();
                    av_packet_make_refcounted(flushPkt);
                    flushPkt->data = nullptr;
                    flushPkt->size = 0;
                    flushPkt->stream_index = videoStreamIdx;
                    flushPkt->flags = 0xFFFF;
                    audioPacketQueue->push(flushPkt, 10);
                }
            }
            isSeekRequested = false;
        }

        if(isEOF) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            LOGI("packet is EOF");
            continue;
        }

        int ret = av_read_frame(fmt_ctx, &packet);
        if (ret < 0) {
            if (ret == AVERROR_EOF || (fmt_ctx->pb && fmt_ctx->pb->eof_reached)) {
                isEOF = true;
                if (videoPacketQueue && hasVideo()) {
                    AVPacket *eofPkt = av_packet_alloc();
                    av_packet_make_refcounted(eofPkt);
                    eofPkt->data = nullptr;
                    eofPkt->size = 0;
                    eofPkt->stream_index = videoStreamIdx;
                    eofPkt->flags = 0xFFFE;
                    videoPacketQueue->push(eofPkt, 10);
                }
                if (audioPacketQueue && hasAudio()) {
                    AVPacket *eofPkt = av_packet_alloc();
                    av_packet_make_refcounted(eofPkt);
                    eofPkt->data = nullptr;
                    eofPkt->size = 0;
                    eofPkt->stream_index = videoStreamIdx;
                    eofPkt->flags = 0xFFFE;
                    audioPacketQueue->push(eofPkt, 10);
                }
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            LOGE("av_read_frame failed, ret = %d, ", ret);
            continue;
        }

        // 将数据包发到对应队列
        if (packet.stream_index == videoStreamIdx && videoPacketQueue) {
            AVPacket *pktCopy = av_packet_alloc();
            av_packet_ref(pktCopy, &packet);

            if (!videoPacketQueue->push(pktCopy, 10)) {
                // 如果push失败
                av_packet_free(&pktCopy);
            }
            videoPacketCount++;
            if (videoPacketQueue->getSize() > 30) {
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
            } else if (videoPacketQueue->getSize() > 80) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        } else if (packet.stream_index == audioStreamIdx) {
            AVPacket *pktCopy = av_packet_alloc();
            av_packet_ref(pktCopy, &packet);

            if (!audioPacketQueue->push(pktCopy, 10)) {
                av_packet_free(&pktCopy);
            }
            audioPacketCount++;
        }
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastLogTime);

        if (elapsed.count() >= 3) {
            LOGI("解封装统计: %d帧/%ds (%.2f帧/秒), 视频队列大小: %d",
                 videoPacketCount, (int)elapsed.count(),
                 videoPacketCount/(float)elapsed.count(),
                 videoPacketQueue->getSize());
            videoPacketCount = 0;
            lastLogTime = now;
        }
//        timer.logElapsed("解封装一帧");
        av_packet_unref(&packet);
    }
}

bool FFmpegDemuxer::hasVideo() const {
    return videoStreamIdx >= 0;
}

bool FFmpegDemuxer::hasAudio() const {
    return audioStreamIdx >= 0;
}
//
// Created by Weichuandong on 2025/3/17.
//

#include "Decoder/FFmpegVideoDecoder.h"

FFmpegVideoDecoder::FFmpegVideoDecoder(std::shared_ptr<SafeQueue<AVPacket *>> pktQueue,
                                       std::shared_ptr<SafeQueue<AVFrame *>> frameQueue) :
    videoPackedQueue(std::move(pktQueue)),
    videoFrameQueue(std::move(frameQueue)),
    avCodecContext(nullptr),
    isPaused(false),
    exitRequested(false),
    mWidth(0),
    mHeight(0),
    format(AV_PIX_FMT_NONE)
{

}

FFmpegVideoDecoder::~FFmpegVideoDecoder() {

}

void FFmpegVideoDecoder::start() {
    if (isRunning()) return;

    LOGI("FFmpegVideoDecoder, start decode thread");
    exitRequested = false;
    isPaused = false;
    isReady = true;

    videoDecodeThread = std::thread(&FFmpegVideoDecoder::videoDecodeThreadFunc, this);
}

void FFmpegVideoDecoder::pause() {
    isPaused = true;
}

void FFmpegVideoDecoder::resume() {
    isPaused = false;
    pauseCond.notify_one();
}

void FFmpegVideoDecoder::stop() {
    exitRequested = true;
    resume();

    if (videoDecodeThread.joinable()) {
        videoDecodeThread.join();
    }
}

void FFmpegVideoDecoder::videoDecodeThreadFunc() {
    if (!avCodecContext || !videoPackedQueue || !videoFrameQueue) {
        LOGE("Video decoder thread: invalid state");
        return;
    }

    AVPacket* packet = nullptr;
    auto lastLogTime = std::chrono::steady_clock::now();
    uint16_t frameCount = 0;

    while (!exitRequested) {
//        PerformanceTimer timer("Decoder");
        // 检查暂停状态
        {
            std::unique_lock<std::mutex> lock(mtx);
            while (isPaused && !exitRequested) {
                pauseCond.wait(lock);
            }
        }

        if (exitRequested) break;

        // 从队列获取数据包
        if (!videoPackedQueue->pop(packet, 10)) {
            continue;
        }

        if (!packet) {
            // 空包，跳过
            continue;
        }

        // 检查特殊包（flush或EOF）
        if (isSpecialPacket(packet)) {
            handleSpecialPacket(packet);
            continue;
        }

        // 发送包到解码器
        int sendResult = avcodec_send_packet(avCodecContext, packet);
        if (sendResult < 0) {
            if (sendResult != AVERROR(EAGAIN)) {
                LOGE("Error sending packet to decoder : %d", sendResult);
            }
            // 释放包
            av_packet_free(&packet);
            continue;
        }

        // 释放
        av_packet_free(&packet);

        // 接收解码后的帧
        AVFrame* frame = av_frame_alloc();
        int receiveResult;

        while ((receiveResult = avcodec_receive_frame(avCodecContext, frame)) >= 0) {
            // 复制帧以便安全地放入队列
            AVFrame* frameCopy = av_frame_alloc();
            if (!frameCopy) {
                av_frame_free(&frame);
                printf("Failed to allocate frame copy\n");
                break;
            }

            av_frame_ref(frameCopy, frame);

//            LOGI("Video: frame->pts = %ld, frame->dts = %ld， time_base.num/time_base.den = %d/%d",
//                 frame->pts, frame->pkt_dts, avCodecContext->time_base.num, avCodecContext->time_base.den);

            // 放入帧队列
            if (!videoFrameQueue->push(frameCopy)) {
                // 队列满，丢弃帧
                av_frame_free(&frameCopy);
                LOGI("Frame queue full, dropping frame\n");
            }
            frameCount++;
            if (videoFrameQueue->getSize() > 30) {
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
            } else if (videoFrameQueue->getSize() > 80) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            // 重用同一个frame对象接收下一帧
            av_frame_unref(frame);
        }

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastLogTime);
        if (elapsed.count() >= 3) {
            LOGI("视频解码统计: %d帧/%ds (%.2f帧/秒), 视频帧队列大小: %d",
                 frameCount, (int)elapsed.count(),
                 frameCount/(float)elapsed.count(),
                 videoFrameQueue->getSize());
            frameCount = 0;
            lastLogTime = now;
        }
//        timer.logElapsed("解码一帧");
        av_frame_free(&frame);

        // 处理接收帧错误
        if (receiveResult != AVERROR(EAGAIN) && receiveResult != AVERROR_EOF) {
            LOGE("Error receiving frame from decoder : %d", receiveResult);
        }
    }

    LOGI("Video decoder thread exited");
}

bool FFmpegVideoDecoder::isSpecialPacket(const AVPacket *packet) const {
    return packet &&
            (packet->data == nullptr &&
             packet->size == 0 &&
            (packet->flags & 0xFFFF) &&
            (packet->flags & 0xFFFE));
}

void FFmpegVideoDecoder::handleSpecialPacket(AVPacket *packet) {
    if (!packet) return;

    if (packet->flags & 0xFFFF) {
        // 刷新解码器内部缓冲区
        avcodec_flush_buffers(avCodecContext);
        videoPackedQueue->flush();
        videoPackedQueue->resume();

        LOGI("Video decoder: flushing buffers");
    }

    if (packet->flags & 0xFFFE) {
        // EOF处理 - 将所有剩余帧输出到队列
        LOGI("Video decoder: EOF packet received");

        AVFrame* frame = av_frame_alloc();
        if (!frame) {
            LOGE("Could not allocate video frame");
            return;
        }

        // 发送空包以获取任何缓冲的帧
        avcodec_send_packet(avCodecContext, nullptr);

        // 接收所有剩余帧
        int ret = 0;
        while (ret >= 0) {
            ret = avcodec_receive_frame(avCodecContext, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                LOGE("Error receiving frame at EOF");
                break;
            }

            // 复制帧以便安全地放入队列
            AVFrame* frameCopy = av_frame_alloc();
            av_frame_ref(frameCopy, frame);

            if (!videoFrameQueue->push(frameCopy)) {
                av_frame_free(&frameCopy);
                LOGE("Frame queue full at EOF, dropping frame");
            }
        }

        av_frame_free(&frame);

        // 向帧队列传递EOF标志
        AVFrame* eofFrame = av_frame_alloc();
        eofFrame->pict_type = AV_PICTURE_TYPE_NONE; // 使用特殊值标记EOF
        if (!videoFrameQueue->push(eofFrame, 10)) {
            av_frame_free(&eofFrame);
        }
    }

    av_packet_free(&packet);
}

bool FFmpegVideoDecoder::configure(const AVCodecParameters *codecParams) {
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

    LOGI("Video decoder configured: %dx%d, pixel format: %d", mWidth, mHeight, format);
    return true;
}



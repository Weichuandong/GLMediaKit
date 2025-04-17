//
// Created by Weichuandong on 2025/4/7.
//

#ifndef GLMEDIAKIT_FFMPEGFRAME_HPP
#define GLMEDIAKIT_FFMPEGFRAME_HPP

#include "interface/IMediaData.h"
#include <memory>

class FFmpegFrame : public IMediaFrame {
public:
    FFmpegFrame(AVFrame* data) :
        frame(data){}

    virtual ~FFmpegFrame(){

    }

    const int64_t getPts() const override { return frame->pts; }

    const PixFormat getPixFormat() const override {
        switch (frame->format) {
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

    const SampleFormat getSampleFormat() const override{
        switch (frame->format) {
            case AV_SAMPLE_FMT_S16:
                return SampleFormat::S16;
            case AV_SAMPLE_FMT_S16P:
                return SampleFormat::S16P;
            default:
                return SampleFormat::UNKNOWN;
        }
    }

    AVFrame * asAVFrame() override{ return frame;}

    static std::shared_ptr<FFmpegFrame> fromAVFrame(AVFrame* frame) {
        AVFrame* clone = av_frame_clone(frame);
        if (!clone) {
            return nullptr;
        }
        av_frame_unref(frame);
        return std::shared_ptr<FFmpegFrame>(std::make_shared<FFmpegFrame>(clone));
    }

    bool createFromYUV420P(const uint8_t* data, int width, int height, int64_t ts) override {
        if (!frame) {
            frame = av_frame_alloc();
        }

        if (!data) {
            return false;
        }

        frame->width = width;
        frame->height = height;
        frame->format = AV_PIX_FMT_YUV420P;
        frame->pts = ts;

        // 分配内存
        int ret = av_frame_get_buffer(frame, 32);
        if (ret < 0) {
            av_frame_free(&frame);
            return false;
        }
        // 确保frame可写
        ret = av_frame_make_writable(frame);
        if (ret < 0) {
            av_frame_free(&frame);
            return false;
        }
        int y_size = width * height;
        int uv_size = y_size / 4;


        memcpy(frame->data[0], data, y_size);
        memcpy(frame->data[1], data + y_size, uv_size);
        memcpy(frame->data[2], data + y_size + uv_size, uv_size);

        return true;
    }

private:
    AVFrame* frame;
};

//class FFmpegFrameFactory {
//public:
//    static std::shared_ptr<FFmpegFrame> create(AVFrame* frame) {
//        return FFmpegFrame::fromAVFrame(frame);
//    }
//};
#endif //GLMEDIAKIT_FFMPEGFRAME_HPP

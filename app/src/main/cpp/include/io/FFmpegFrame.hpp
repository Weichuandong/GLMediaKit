//
// Created by Weichuandong on 2025/4/7.
//

#ifndef GLMEDIAKIT_FFMPEGFRAME_HPP
#define GLMEDIAKIT_FFMPEGFRAME_HPP

#include "interface/IMediaData.h"

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

private:
    AVFrame* frame;
};
#endif //GLMEDIAKIT_FFMPEGFRAME_HPP

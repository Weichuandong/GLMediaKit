//
// Created by Weichuandong on 2025/4/7.
//

#ifndef GLMEDIAKIT_IMEDIADATA_H
#define GLMEDIAKIT_IMEDIADATA_H

#include <cstdint>
extern "C"{
#include <libavcodec/avcodec.h>
}
/**
 * 保存编码数据
 * */
enum class PixFormat {
    UNKNOWN,
    YUV420P,
    NV12,
    RGBA24
};

enum class SampleFormat {
    UNKNOWN,
    S16,
    S16P
};


class IMediaPacket {
public:
    ~IMediaPacket() = default;

    virtual const uint8_t* getData() const = 0;
    virtual const size_t getSize() const = 0;
    virtual const int64_t getPts() const = 0;

    virtual AVPacket* asAVPacket() const = 0;
    virtual const int64_t getTs() = 0;
};

class IMediaFrame {
public:
    ~IMediaFrame() = default;

    virtual const int64_t getPts() const = 0;
    // 视频特有属性
//    virtual const int getWidth() const = 0;
//    virtual const int getHeight() const = 0;
    virtual const PixFormat getPixFormat() const = 0;
//
//    // 音频特有属性
    virtual const SampleFormat getSampleFormat() const = 0;

    virtual AVFrame * asAVFrame() = 0;

    virtual bool createFromYUV420P(const uint8_t* data, int width, int height, int64_t ts) = 0;
};


#endif //GLMEDIAKIT_IMEDIADATA_H

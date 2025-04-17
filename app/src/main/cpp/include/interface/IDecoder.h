//
// Created by Weichuandong on 2025/3/26.
//

#ifndef GLMEDIAKIT_IDECODER_H
#define GLMEDIAKIT_IDECODER_H

#include <unordered_map>
#include <vector>
#include <string>
#include "interface/IMediaData.h"

extern "C" {
#include "ffmpeg/include/libavformat/avformat.h"
#include "ffmpeg/include/libavcodec/avcodec.h"
}

class DecoderConfig {
public:
    // 解码器类型
    std::string type;

    // 额外初始化配置
    std::unordered_map<std::string, std::vector<uint8_t>> extraData;

    // 附加参数
    std::unordered_map<std::string, std::string> parameters;

    // FFmpeg相关配置
    AVCodecParameters* param;

    // 视频宽高
    int width;
    int height;

    PixFormat format;

    PixFormat fromAVFormat(AVPixelFormat format1) {
        switch (format1) {
            case AV_PIX_FMT_YUV420P:
                return PixFormat::YUV420P;
            case AV_PIX_FMT_NV12:
                return PixFormat::NV12;
            case AV_PIX_FMT_RGB24:
                return PixFormat::RGBA24;
            default:
                return PixFormat::UNKNOWN;
        }
    }
};


class IDecoder {
public:
    virtual ~IDecoder() = default;

    virtual int SendPacket(const std::shared_ptr<IMediaPacket>& packet) = 0;

    virtual int ReceiveFrame(std::shared_ptr<IMediaFrame>& frame) = 0;

    virtual bool isReadying() = 0;

    virtual bool configure(const DecoderConfig& config) = 0;
};


class IVideoDecoder : public IDecoder  {
public:
    virtual ~IVideoDecoder() = default;

    virtual int getWidth() = 0;

    virtual int getHeight() = 0;

    virtual PixFormat getPixFormat() = 0;
};

class IAudioDecoder : public IDecoder {
public:
    virtual ~IAudioDecoder() = default;

    virtual int getSampleRate() = 0;

    virtual int getChannel() = 0;

    virtual SampleFormat getSampleFormat() = 0;
};

#endif //GLMEDIAKIT_IDECODER_H

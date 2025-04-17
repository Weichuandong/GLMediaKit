//
// Created by Weichuandong on 2025/4/7.
//

#ifndef GLMEDIAKIT_MEDIACODECVIDEODECODER_H
#define GLMEDIAKIT_MEDIACODECVIDEODECODER_H

#include "interface/IDecoder.h"
#include "Decoder/MediaCodecDecoderWrapper.h"
#include <string>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "C++_MediaCodecVideoDecoder", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "C++_MediaCodecVideoDecoder", __VA_ARGS__)

class MediaCodecVideoDecoder : public IVideoDecoder {
public:
    MediaCodecVideoDecoder();

    virtual ~MediaCodecVideoDecoder() = default;

    int SendPacket(const std::shared_ptr<IMediaPacket>& packet) override;

    int ReceiveFrame(std::shared_ptr<IMediaFrame>& frame) override;

    bool isReadying() override;

    bool configure(const DecoderConfig& config) override;

    int getWidth() override;

    int getHeight() override;

    PixFormat getPixFormat() override;

private:
    DecoderConfig decoderConfig;

    std::unique_ptr<MediaCodecDecoderWrapper> mediaCodecDecoderWrapper;

    std::atomic<bool> ready;

    // 视频属性
    int mWidth;
    int mHeight;
    PixFormat format;

    std::vector<uint8_t> convertAVCCToAnnexB(std::vector<uint8_t>& src);
    std::vector<uint8_t> convertIDRAVCCToAnnexB(std::vector<uint8_t>& src, std::vector<uint8_t>& sps, std::vector<uint8_t>& pps);
    bool isIDRFrame(AVPacket* packet);

};

#endif //GLMEDIAKIT_MEDIACODECVIDEODECODER_H

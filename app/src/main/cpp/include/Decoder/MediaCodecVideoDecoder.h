//
// Created by Weichuandong on 2025/4/7.
//

#ifndef GLMEDIAKIT_MEDIACODECVIDEODECODER_H
#define GLMEDIAKIT_MEDIACODECVIDEODECODER_H

#include "interface/IDecoder.h"

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

};

#endif //GLMEDIAKIT_MEDIACODECVIDEODECODER_H

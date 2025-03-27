//
// Created by Weichuandong on 2025/3/26.
//

#ifndef GLMEDIAKIT_IDECODER_H
#define GLMEDIAKIT_IDECODER_H

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
};

class IDecoder {
public:
    virtual ~IDecoder() = default;

    virtual int SendPacket(const AVPacket* packet) = 0;

    virtual int ReceiveFrame(AVFrame* frame) = 0;

    virtual bool isReadying() = 0;

    virtual bool configure(const AVCodecParameters* codecParams) = 0;
};


class IVideoDecoder : public IDecoder  {
public:
    virtual ~IVideoDecoder() = default;

    virtual int getWidth() = 0;

    virtual int getHeight() = 0;
};

class IAudioDecoder : public IDecoder {
public:
    virtual ~IAudioDecoder() = default;

    virtual int getSampleRate() = 0;

    virtual int getChannel() = 0;

    virtual int getSampleFormat() = 0;
};

#endif //GLMEDIAKIT_IDECODER_H

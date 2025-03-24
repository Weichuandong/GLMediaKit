//
// Created by Weichuandong on 2025/3/19.
//

#ifndef GLMEDIAKIT_IVIDEODECODER_H
#define GLMEDIAKIT_IVIDEODECODER_H

extern "C"{
#include <libavcodec/avcodec.h>
#include "libavformat/avformat.h"

};
#include <string>

class IVideoDecoder {
public:
    virtual ~IVideoDecoder() = default;

    virtual bool configure(const AVCodecParameters* codecParams) = 0;

//    virtual int open() = 0;
//
//    virtual int sendPacket(const AVPacket* packet) = 0;
//
//    virtual int receiveFrame() = 0;

    virtual void start() = 0;

    virtual void pause() = 0;

    virtual void resume() = 0;

    virtual void stop() = 0;

    virtual bool isRunning() = 0;

    virtual int getWidth() = 0;

    virtual int getHeight() = 0;
};

#endif //GLMEDIAKIT_IVIDEODECODER_H

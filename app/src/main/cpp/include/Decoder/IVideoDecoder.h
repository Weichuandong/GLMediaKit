//
// Created by Weichuandong on 2025/3/19.
//

#ifndef GLMEDIAKIT_IVIDEODECODER_H
#define GLMEDIAKIT_IVIDEODECODER_H

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}
#include <string>

class IVideoDecoder {
public:
    virtual ~IVideoDecoder() = default;

    virtual bool configure(const AVCodecParameters* codecParams) = 0;

    virtual void start() = 0;

    virtual void pause() = 0;

    virtual void resume() = 0;

    virtual void stop() = 0;

    virtual bool isRunning() = 0;

    virtual bool isReadying() = 0;

    virtual int getWidth() = 0;

    virtual int getHeight() = 0;

    virtual int getSampleRate() = 0;

    virtual int getChannel() = 0;

    virtual int getSampleFormat() = 0;
};

#endif //GLMEDIAKIT_IVIDEODECODER_H

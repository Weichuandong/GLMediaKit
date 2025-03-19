//
// Created by Weichuandong on 2025/3/17.
//

#ifndef GLMEDIAKIT_FFMPEGVIDEODECODER_H
#define GLMEDIAKIT_FFMPEGVIDEODECODER_H

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
};
#include <android/log.h>

#include "IVideoDecoder.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "FFmpegVideoDecoder", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "FFmpegVideoDecoder", __VA_ARGS__)

class FFmpegVideoDecoder : public IVideoDecoder {
public:

};

#endif //GLMEDIAKIT_FFMPEGVIDEODECODER_H

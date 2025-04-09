//
// Created by Weichaundong on 2025/4/9.
//

#ifndef GLMEDIAKIT_MEDIACODECFRAME_HPP
#define GLMEDIAKIT_MEDIACODECFRAME_HPP

#include "interface/IMediaData.h"

class MediaCodecFrame : public IMediaFrame {
public:
    MediaCodecFrame();

    const int64_t getPts() const override {

    }
    const PixFormat getPixFormat() const override {

    }
    const SampleFormat getSampleFormat() const override {

    }

    AVFrame * asAVFrame() override {
        return nullptr;
    }

};
#endif //GLMEDIAKIT_MEDIACODECFRAME_HPP

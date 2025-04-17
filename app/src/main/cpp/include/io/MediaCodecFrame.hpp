//
// Created by Weichaundong on 2025/4/9.
//

#ifndef GLMEDIAKIT_MEDIACODECFRAME_HPP
#define GLMEDIAKIT_MEDIACODECFRAME_HPP

#include "interface/IMediaData.h"
#include "jni.h"
#include <vector>

class MediaCodecFrame : public IMediaFrame {
public:
    MediaCodecFrame() {

    }

    const int64_t getPts() const override {
        return pts;
    }
    const PixFormat getPixFormat() const override {
        return format;
    }
    const SampleFormat getSampleFormat() const override {
        return SampleFormat::UNKNOWN;
    }

    AVFrame * asAVFrame() override {
        return nullptr;
    }


private:
//    jobject buffer = nullptr;
    int width = 0;
    int height = 0;
    int64_t pts = 0;
    PixFormat format = PixFormat::UNKNOWN;

    // 平面数据缓存
    std::vector<std::vector<uint8_t>> planeData;
    std::vector<int> planeStrides;

};
#endif //GLMEDIAKIT_MEDIACODECFRAME_HPP

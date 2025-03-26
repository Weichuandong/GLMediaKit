//
// Created by Weichuandong on 2025/3/26.
//

#ifndef GLMEDIAKIT_ICLOCK_H
#define GLMEDIAKIT_ICLOCK_H

extern "C" {
#include <libavutil/time.h>
};

struct IClock {
    double pts = 0.0;               // 当前播放时间
    double lastUpdateTime = 0.0;    // 上次更新的系统时间

    void update(const IClock& clock) {
        this->pts = clock.pts;
        this->lastUpdateTime = clock.lastUpdateTime;
    }

    double getCurrentTime() {
        double elapsed = av_gettime() / 1000000.0 - lastUpdateTime; // 当前时间与上次更新的时间差
        return pts + elapsed; // 返回估计的当前时间
    }
};
#endif //GLMEDIAKIT_ICLOCK_H

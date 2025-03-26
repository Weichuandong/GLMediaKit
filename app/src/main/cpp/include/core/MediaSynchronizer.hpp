//
// Created by Weichuandong on 2025/3/26.
//

#ifndef GLMEDIAKIT_MEDIASYNCHRONIZER_HPP
#define GLMEDIAKIT_MEDIASYNCHRONIZER_HPP

#include "core/IClock.h"

#include <mutex>

class MediaSynchronizer {
public:
    enum class SyncSource { AUDIO, VIDEO, EXTERNAL };

    explicit MediaSynchronizer(SyncSource master = SyncSource::AUDIO) :
        currentMaster(master)
    {
        
    }

    void update(const IClock& clock, SyncSource type) {
        switch (type) {
            case SyncSource::AUDIO:
                return audioClock.update(clock);
            case SyncSource::VIDEO:
                return videoClock.update(clock);
            case SyncSource::EXTERNAL:
                return externalClock.update(clock);
        }
    }

    double getCurrentTime() {
        switch (currentMaster) {
            case SyncSource::AUDIO:
                return audioClock.getCurrentTime();
            case SyncSource::VIDEO:
                return videoClock.getCurrentTime();
            case SyncSource::EXTERNAL:
                return externalClock.getCurrentTime();
        }
    }

private:
    IClock audioClock;
    IClock videoClock;
    IClock externalClock;

    std::mutex clockMtx;
    SyncSource currentMaster;
};

#endif //GLMEDIAKIT_MEDIASYNCHRONIZER_HPP

//
// Created by Weichuandong on 2025/3/24.
//

#ifndef GLMEDIAKIT_PERFORMCETIMER_HPP
#define GLMEDIAKIT_PERFORMCETIMER_HPP

#include <chrono>
#include <string>
#include <utility>
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "PerformanceTimer", __VA_ARGS__)

class PerformanceTimer {
private:
    std::chrono::steady_clock::time_point startTime;
    std::string operationName;
    bool isLogging;

public:
    explicit PerformanceTimer(const std::string& name, bool enableLog = true)
            : operationName(name), isLogging(enableLog) {
        startTime = std::chrono::steady_clock::now();
    }

    ~PerformanceTimer() {
        if (isLogging) {
            logElapsed("完成");
        }
    }

    void logElapsed(const std::string& checkpoint) {
        if (!isLogging) return;

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
                (now - startTime).count();
        LOGI("性能[%s]: %s - %lld ms", operationName.c_str(),
             checkpoint.c_str(), (long long)elapsed);
    }
};


#endif //GLMEDIAKIT_PERFORMCETIMER_HPP

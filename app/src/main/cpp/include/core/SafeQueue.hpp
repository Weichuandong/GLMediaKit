//
// Created by Weichuandong on 2025/3/20.
//

#ifndef GLMEDIAKIT_SAFEQUEUE_HPP
#define GLMEDIAKIT_SAFEQUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

extern "C" {
#include <libavcodec/avcodec.h>
};
template<typename T>
class SafeQueue {
public:
    explicit SafeQueue(size_t maxSize = 3) :
        maxSize(maxSize),
        flushing(false){}

    bool push(const T& item, int timeoutMs = -1) {
        std::unique_lock<std::mutex> lock(mtx);

        if (timeoutMs > 0) {
            // 等待有空间可用或刷新
            if (!spaceCond.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                                    [this]() { return queue.size() < maxSize || flushing; })) {
                return false;
            }
        } else {
            spaceCond.wait(lock, [this](){ return queue.size() < maxSize || flushing; });
        }

        if (flushing) return false;

        queue.emplace(item);
        dataCond.notify_one();
        return true;
    }

    bool pop(T& item, int timeoutMs = -1) {
        std::unique_lock<std::mutex> lock(mtx);

        if (timeoutMs > 0) {
            if (!dataCond.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                                   [this]() { return !queue.empty() || flushing; })) {
                return false;
            }
        } else {
            dataCond.wait(lock, [this]() { return !queue.empty() || flushing; });
        }

        if (queue.empty()) return false;

        item = std::move(queue.front());
        queue.pop();
        spaceCond.notify_one();
        return true;
    }

    void flush() {
        std::unique_lock<std::mutex> lock(mtx);
        flushing = true;

        while (!queue.empty()) {
            T item = std::move(queue.front());
            queue.pop();

            if constexpr (std::is_pointer<T>::value) {
                if constexpr (std::is_same<T, AVPacket*>::value) {
                    av_packet_free(&item);
                } else if constexpr (std::is_same<T, AVFrame*>::value) {
                    av_frame_free(&item);
                } else {
                    delete item;
                }
            }
        }

        dataCond.notify_all();
        spaceCond.notify_all();
    }

    void resume() {
        std::unique_lock<std::mutex> lock(mtx);
        flushing = false;
    }

    int getSize() {
        std::unique_lock<std::mutex> lock(mtx);
        return queue.size();
    }

private:
    std::queue<T> queue;
    std::mutex mtx;
    std::condition_variable dataCond;
    std::condition_variable spaceCond;
    size_t maxSize;
    std::atomic<bool> flushing;
};

#endif //GLMEDIAKIT_SAFEQUEUE_HPP

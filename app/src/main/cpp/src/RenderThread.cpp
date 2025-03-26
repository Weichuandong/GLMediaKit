//
// Created by Weichuandong on 2025/3/10.
//

#include "RenderThread.h"

RenderThread::RenderThread(std::shared_ptr<SafeQueue<AVFrame*>> frameQueue,
                           std::shared_ptr<MediaSynchronizer> sync) :
    videoFrameQueue(std::move(frameQueue)),
    renderer(nullptr),
    eglCore(nullptr),
    synchronizer(std::move(sync))
{

}

RenderThread::~RenderThread() {
    stop();
}

void RenderThread::start(IRenderer* r, EGLCore* core, AVRational rational) {
    if (exitRequest) {
        return;
    }
    LOGI("RenderThread : start render thread");

    if (!r || !core) {
        LOGE("eglcore or renderer is null");
        return;
    }

    videoTimeBase = rational;
    isPaused = false;
    renderer = r;
    eglCore = core;
    exitRequest = false;

    isReady = true;
    thread = std::thread(&RenderThread::renderLoop, this);
}

void RenderThread::stop() {
    if (exitRequest) {
        return;
    }

    exitRequest = true;
    if (thread.joinable()) {
        thread.join();
    }
}

bool RenderThread::isRunning() const {
    return !isPaused && thread.joinable();
}

void RenderThread::renderLoop() {
    LOGI("Render loop started");

    // 确保在渲染线程中使用EGL上下文
    if(!eglCore->makeCurrent()) {
        LOGE("Can not makeCurrent");
        return;
    }

    if (renderer) {
        if (!renderer->init()) {
            LOGE("Failed to init renderer");
            return;
        }
    }

    auto lastLogTime = std::chrono::steady_clock::now();
    uint16_t renderFrameCount = 0;
    double syncThreshold = 0.02;   // 20ms同步阈值

    while (!exitRequest) {
        //判断是否暂停
        {
            std::unique_lock<std::mutex> lock(mtx);
            while (isPaused && !exitRequest) {
                pauseCond.wait(lock);
            }
        }

        executeGLTasks();

        if (renderer) {
            // 取AVFrame
            AVFrame* frame{nullptr};
            videoFrameQueue->pop(frame, 1);

            if (frame && frame->width && frame->height) {
                // 添加时钟同步逻辑
                double masterTime = synchronizer ? synchronizer->getCurrentTime() : videoClock.getCurrentTime();

                // 更新视频时钟
                if (frame->pts != AV_NOPTS_VALUE) {
                    videoClock.pts = frame->pts * av_q2d(videoTimeBase);
                    videoClock.lastUpdateTime = av_gettime() / 1000000.0;
                    synchronizer->update(videoClock, MediaSynchronizer::SyncSource::VIDEO);
                }

                // 计算时间差值
                double diff = videoClock.pts - masterTime;
                LOGD("diff = %lf, videoPts = %lf, masterTime = %lf", diff, videoClock.pts, masterTime);

                if (diff <= -syncThreshold) {
                    // 视频慢
                    LOGD("video is %lfS slow", fabs(diff));
                    renderer->onDrawFrame(frame);

                    // 如果视频极其落后
                    if (diff < -3 * syncThreshold && videoFrameQueue->getSize() > 1) {
                        LOGW("视频严重落后，考虑丢帧");
                        // 丢帧逻辑

                    }
                } else if (diff >= syncThreshold) {
                    // 视频快
                    int waitTime = std::min(100, (int)(diff * 1000));
                    LOGD("video is %lfS fast, sleep %dms", fabs(diff), waitTime);

                    std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
                    renderer->onDrawFrame(frame);
                } else {
                    LOGD("audio and video synchronization");
                    renderer->onDrawFrame(frame);
                }
            } else {
                // frame无效

            }
        }

        // 判断egl是否有效
        if (eglCore == nullptr || eglCore->getSurface() == EGL_NO_SURFACE) {
            LOGE("当前eglCore无效或未绑定Surface");
            continue;
        }
        // 交换缓冲区
        eglCore->swapBuffers();

        renderFrameCount++;
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastLogTime);
        if (elapsed.count() >= 3000) {
            double elapsedSeconds = elapsed.count() / 1000.0;
            LOGI("渲染统计: %d帧/%.3f秒 (%.2f帧/秒) [精确毫秒:%lld]",
                 renderFrameCount, elapsedSeconds,
                 renderFrameCount/elapsedSeconds,
                 elapsed.count());
            renderFrameCount = 0;
            lastLogTime = now;
        }
    }
    LOGI("Render loop stopped");
}

void RenderThread::pause() {
    isPaused = true;
}

void RenderThread::resume() {
    isPaused = false;
    pauseCond.notify_one();
}

void RenderThread::executeGLTasks() {
    std::unique_lock<std::mutex> lk(taskMtx);
    while (!glTasks.empty()) {
        auto task = glTasks.front();
        glTasks.pop();

        lk.unlock();
        task();
        lk.lock();
    }
}

// 提交需要在GL线程上执行的任务
void RenderThread::postTask(const std::function<void()>& task) {
    std::lock_guard<std::mutex> lock(taskMtx);
    glTasks.emplace(task);
}

bool RenderThread::isReadying() const {
    return isReady;
}

//
// Created by Weichuandong on 2025/3/21.
//

#ifndef GLMEDIAKIT_VIDEORENDERER_H
#define GLMEDIAKIT_VIDEORENDERER_H

#include "interface/IRenderer.h"
#include "ShaderManager.h"
#include "OffscreenRenderer.h"
#include "EGL/EGLCore.h"
#include "core/PerformceTimer.hpp"

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "VideoRenderer", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "VideoRenderer", __VA_ARGS__)

class VideoRenderer : public IRenderer {
public:
    explicit VideoRenderer();

    ~VideoRenderer() override;

    // 初始化渲染器
    bool init() override;

    // 处理视口变化
    void onSurfaceChanged(int width, int height) override;

    // 绘制一帧
    void onDrawFrame() override;
    void onDrawFrame(AVFrame* frame) override;

    // 释放资源
    void release() override;

    void setScaleMode(ScalingMode mode_) override;

private:
    ScalingMode mode;

    ShaderManager shaderManager;

    GLuint program{0};
    GLuint vao{0};
    GLuint vbo{0};
    GLuint y_tex = 0, u_tex = 0, v_tex = 0;

    OffscreenRenderer offscreenRenderer;

    int videoWidth{0};
    int videoHeight{0};

    int surfaceWidth{0};
    int surfaceHeight{0};

    // 着色器源码
    const char* getVertexShaderSource() const;
    const char* getFragmentShaderSource() const;

    //
    void create_textures();
    void update_textures(AVFrame* frame);

    void calculateDisplayGeometry();
    bool geometryNeedsChange{false};
};

#endif //GLMEDIAKIT_VIDEORENDERER_H

//
// Created by Weichuandong on 2025/3/10.
//

#ifndef GLMEDIAKIT_IRENDERER_H
#define GLMEDIAKIT_IRENDERER_H

#include <EGL/egl.h>

class IRenderer {
public:
    enum class ScalingMode {
        FIT,    // 适应模式（保持比例，可能有黑边）
        FILL,   // 填充模式（保持比例，可能裁剪）
        STRETCH // 拉伸模式（忽略比例，填满屏幕）
    };

    virtual ~IRenderer() = default;

    // 初始化渲染器
    virtual bool init() = 0;

    // 处理视口变化
    virtual void onSurfaceChanged(int width, int height) = 0;

    // 绘制一帧
    virtual void onDrawFrame() = 0;

    // 释放资源
    virtual void release() = 0;

    virtual void setScaleMode(ScalingMode mode) = 0;
};

#endif //GLMEDIAKIT_IRENDERER_H
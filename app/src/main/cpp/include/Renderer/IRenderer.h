//
// Created by Weichuandong on 2025/3/10.
//

#ifndef LEARNOPENGLES_IRENDERER_H
#define LEARNOPENGLES_IRENDERER_H

#include <EGL/egl.h>

class IRenderer {
public:
    virtual ~IRenderer() = default;

    // 初始化渲染器
    virtual bool init() = 0;

    // 处理视口变化
    virtual void onSurfaceChanged(int width, int height) = 0;

    // 绘制一帧
    virtual void onDrawFrame() = 0;

    // 释放资源
    virtual void release() = 0;

};

#endif //LEARNOPENGLES_IRENDERER_H
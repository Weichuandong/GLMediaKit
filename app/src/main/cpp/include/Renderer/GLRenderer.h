//
// Created by Weichuandong on 2025/3/8.
//

#ifndef GLMEDIAKIT_GLRENDERER_H
#define GLMEDIAKIT_GLRENDERER_H

#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <android/log.h>
#include <memory>

#include "Geometry/Geometry.h"
#include "ShaderManager.h"
#include "IRenderer.h"
#include "Geometry/Triangle.h"
#include "Geometry/Square.h"
#include "Geometry/MovingTriangle.h"
#include "Geometry/RotatingTriangle.h"

#define LOG_TAG "GLRenderer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class GLRenderer: public IRenderer{
public:

    GLRenderer();
    ~GLRenderer() override = default;

    bool init() override;

    void onSurfaceChanged(int width, int height) override;

    void onDrawFrame() override;

    bool setGeometry(const std::shared_ptr<Geometry>& geometry_);

    void release() override;

private:
    std::unique_ptr<ShaderManager> shaderManager;

    std::shared_ptr<Geometry> geometry;

    GLuint program;

    uint16_t mWidth;
    uint16_t mHeight;
};

#endif //GLMEDIAKIT_GLRENDERER_H

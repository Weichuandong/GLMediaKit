//
// Created by Weichuandong on 2025/3/10.
//

#ifndef GLMEDIAKIT_IMAGERENDERER_H
#define GLMEDIAKIT_IMAGERENDERER_H

#include "IRenderer.h"
#include <cstdlib>
#include <cstring>
#include <GLES3/gl3.h>
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "ImageRenderer", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "ImageRenderer", __VA_ARGS__)

class ImageRenderer : public IRenderer {
public:
    ImageRenderer();
    ~ImageRenderer();

    bool init() override;
    void onSurfaceChanged(int width, int height) override;
    void onDrawFrame() override;
    void release() override;
    // 设置图像数据
    void setImage(void* data, int width, int height);

private:
    int width;
    int height;
    void* imageBuffer;
    int imageWidth;
    int imageHeight;

    bool hasNewImage;

    // OpenGL 相关
    GLuint textureId;
    GLuint programId;

    void generateDefaultImage();
    GLuint loadShaders();
};

#endif //GLMEDIAKIT_IMAGERENDERER_H

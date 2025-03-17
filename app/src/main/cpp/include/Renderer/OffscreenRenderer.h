//
// Created by Weichuandong on 2025/3/17.
//

#ifndef GLMEDIAKIT_OFFSCREENRENDERER_H
#define GLMEDIAKIT_OFFSCREENRENDERER_H

#include <GLES3/gl3.h>
#include "Renderer/ShaderManager.h"

class OffscreenRenderer {
public:
    OffscreenRenderer();

    // 初始化离屏渲染资源
    bool initialize(int w, int h);

    // 开始离屏渲染
    void beginRender();

    // 结束离屏渲染并绘制到默认帧缓冲
    void endRenderAndDraw(int screenWidth, int screenHeight);

    // 结束离屏渲染但不绘制到屏幕
    void endRender();

    // 将结果绘制到当前激活的帧缓冲
    void drawToCurrentFramebuffer();

    // 获取渲染结果纹理ID
    GLuint getOutputTexture() const { return textureColor; }

    // 清理资源
    void cleanup();

    // 检查是否已初始化
    bool isInitialized() const { return initialized; }

private:
    GLuint fbo;
    GLuint textureColor;
    GLuint rbo;
    GLuint screenVAO, screenVBO;
    GLuint screenShader;

    int width, height;
    bool initialized;

    ShaderManager shaderManager;
    const char* getVertexShaderSource() const;
    const char* getFragmentShaderSource() const;
};


#endif //GLMEDIAKIT_OFFSCREENRENDERER_H

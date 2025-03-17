//
// Created by Weichuandong on 2025/3/17.
//

#include "Renderer/OffscreenRenderer.h"

OffscreenRenderer::OffscreenRenderer() :
    fbo(0),
    textureColor(0),
    rbo(0),
    screenVAO(0),
    screenVBO(0),
    screenShader(0),
    initialized(false),
    width(0),
    height(0) {}

bool OffscreenRenderer::initialize(int w, int h) {
    // 保存尺寸
    width = w;
    height = h;

    // 创建帧缓冲对象
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // 创建颜色纹理附件
    glGenTextures(1, &textureColor);
    glBindTexture(GL_TEXTURE_2D, textureColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColor, 0);

    // 创建渲染缓冲对象(可选，用于深度和模板缓冲)
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    // 检查帧缓冲是否完整
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        // 处理错误
        return false;
    }

    // 解绑帧缓冲
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 创建用于显示纹理的屏幕四边形
    float quadVertices[] = {
            // 位置         // 纹理坐标
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
            1.0f, -1.0f,  1.0f, 0.0f,
            -1.0f,  1.0f,  0.0f, 1.0f,
            1.0f, -1.0f,  1.0f, 0.0f,
            1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &screenVAO);
    glGenBuffers(1, &screenVBO);
    glBindVertexArray(screenVAO);
    glBindBuffer(GL_ARRAY_BUFFER, screenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    // 创建屏幕着色器
    screenShader = shaderManager.createProgram(
            getVertexShaderSource(),
            getFragmentShaderSource()
    );  // 这个函数需要单独实现

    initialized = true;
    return true;
}

void OffscreenRenderer::beginRender() {
    if (!initialized) return;

    // 绑定到离屏帧缓冲
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, width, height);
    // 添加清除缓冲的代码
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OffscreenRenderer::endRender() {
    if (!initialized) return;

    // 解绑帧缓冲
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OffscreenRenderer::endRenderAndDraw(int screenWidth, int screenHeight) {
    if (!initialized) return;

    // 首先结束离屏渲染
    endRender();

    // 然后设置视口为屏幕大小
    glViewport(0, 0, screenWidth, screenHeight);

    // 绘制离屏渲染结果到当前帧缓冲
    drawToCurrentFramebuffer();
}

void OffscreenRenderer::drawToCurrentFramebuffer() {
    if (!initialized) return;

    // 使用屏幕着色器
    glUseProgram(screenShader);

    // 绑定离屏渲染的结果纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureColor);
    // 设置纹理采样器
    glUniform1i(glGetUniformLocation(screenShader, "screenTexture"), 0);

    // 绘制全屏四边形
    glBindVertexArray(screenVAO);
    glDisable(GL_DEPTH_TEST);  // 关闭深度测试
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glEnable(GL_DEPTH_TEST);   // 恢复深度测试
    glBindVertexArray(0);
}

void OffscreenRenderer::cleanup() {
    if (!initialized) return;

    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &textureColor);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteVertexArrays(1, &screenVAO);
    glDeleteBuffers(1, &screenVBO);
    glDeleteProgram(screenShader);

    initialized = false;
}

const char *OffscreenRenderer::getVertexShaderSource() const {
    return R"(#version 300 es
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
void main() {
    gl_Position = vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";
}

const char *OffscreenRenderer::getFragmentShaderSource() const {
    return R"(#version 300 es
precision mediump float;
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D screenTexture;
void main() {
    FragColor = texture(screenTexture, TexCoord);
}
)";
}
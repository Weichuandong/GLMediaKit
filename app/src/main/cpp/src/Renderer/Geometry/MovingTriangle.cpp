//
// Created by Weichuandong on 2025/3/13.
//

#include "Renderer/Geometry/MovingTriangle.h"

void MovingTriangle::init() {
    // 三角形顶点数据
    float vertices[] = {
            // 位置              // 颜色
            0.0f,  0.15f, 0.0f,  1.0f, 0.0f, 0.0f,  // 顶点
            -0.15f, -0.15f, 0.0f,  0.0f, 1.0f, 0.0f,  // 左下
            0.15f, -0.15f, 0.0f,  0.0f, 0.0f, 1.0f   // 右下
    };

    // 创建并配置VAO和VBO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 颜色属性
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 解绑
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // 初始化时间和矩阵
    mLastTime = std::chrono::high_resolution_clock::now();
}

void MovingTriangle::draw() {
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // 解绑
    glBindVertexArray(0);
}

const char *MovingTriangle::getVertexShaderSource() const {
    return R"(#version 300 es
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
uniform vec2 uTranslation;
out vec3 vColor;
void main() {
    gl_Position = vec4(aPos.x + uTranslation.x, aPos.y + uTranslation.y, aPos.z, 1.0);
    vColor = aColor;
}
)";
}

const char *MovingTriangle::getFragmentShaderSource() const {
    return R"(#version 300 es
precision mediump float;
in vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor, 1.0);
}
)";
}

void MovingTriangle::setUniform(GLuint program) {
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - mLastTime).count();
    mLastTime = currentTime;

    mTotalTime += deltaTime;

    mTranslationX += mSpeedX * sin(deltaTime);
    mTranslationY += mSpeedY * cos(deltaTime);

    if (mTranslationX > 0.85f || mTranslationX < -0.85f) {
        mTranslationX = 0.85f;
        mSpeedX = -mSpeedX;
    }
    if (mTranslationY > 0.85f || mTranslationY < -0.85f) {
        mTranslationY = 0.85f;
        mSpeedY = -mSpeedY;
    }

    glUniform2f(glGetUniformLocation(program, "uTranslation"), mTranslationX, mTranslationY);
}
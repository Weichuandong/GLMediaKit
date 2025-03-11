//
// Created by Weichuandong on 2025/3/10.
//
#include "Renderer/Geometry/Triangle.h"

void Triangle::init() {
//    Geometry::init();
    float vertices[] = {
            -0.8f, -0.8f, 0.0f,
            0.8f, -0.8f, 0.0f,
            0.0f, 0.8f, 0.0f
    };

    // 创建并配置VAO和VBO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    // 解绑
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    LOGI("Triangle data prepared");
}

void Triangle::bind() {
//    Geometry::bind();
    glBindVertexArray(vao);
}

void Triangle::draw() {
//    Geometry::draw();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

const char *Triangle::getVertexShaderSource() const {
    return R"(#version 300 es
layout (location = 0) in vec3 aPos;
void main() {
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
)";
}

const char *Triangle::getFragmentShaderSource() const {
    return R"(#version 300 es
precision mediump float;
out vec4 FragColor;
void main() {
    FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}
)";
}

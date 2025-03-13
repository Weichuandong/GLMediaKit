//
// Created by Weichuandong on 2025/3/13.
//
#include "Renderer/Geometry/Square.h"

void Square::init() {
    float vertices[] = {
            0.5f, 0.5f, 0.0f,     // 右上
            -0.5f, 0.5f, 0.0f,   // 左上
            -0.5f, -0.5f, 0.0f,  // 左下
            0.5f, -0.5f, 0.0f   // 右下
    };

    unsigned int indices[] = {
            0, 1, 2,
            2, 3, 0
    };
    // 创建并配置vao
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // 创建并配置vbo
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 创建并配置ebo
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices),indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    // 解绑
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    LOGI("Square data prepared");
}

void Square::bind() {
    glBindVertexArray(vao);
}

void Square::draw() {
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

void Square::cleanup() {
}

const char *Square::getVertexShaderSource() const {
    return R"(#version 300 es
layout (location = 0) in vec3 aPos;
void main() {
    gl_Position = vec4(aPos, 1.0);
}
)";
}

const char *Square::getFragmentShaderSource() const {
    return R"(#version 300 es
precision mediump float;
out vec4 FragColor;
void main() {
    FragColor = vec4(0.0f, 1.0f, 0.0f, 0.0f);
}
)";
}



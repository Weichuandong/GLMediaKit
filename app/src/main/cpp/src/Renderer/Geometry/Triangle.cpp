//
// Created by Weichuandong on 2025/3/10.
//
#include "Renderer/Geometry/Triangle.h"

void Triangle::init() {
    float vertices[] = {
            -0.8f, -0.8f, 0.0f,
            0.8f, -0.8f, 0.0f,
            0.0f, 0.8f, 0.0f
    };

    // 绘制两个顶点相连的三角形
//    float twoVertices[] = {
//            0.5f, 0.5f, 0.0f,
//            -0.5f, 0.5f, 0.0f,
//            0.0f, 0.0f, 0.0f,
//            -0.5f, -0.5f, 0.0f,
//            0.5f, -0.5f, 0.0f,
//            0.0f,0.0f,0.0f
//    };

    // 添加颜色属性
    float colorVertices[] = {
            0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,      // 右下
            -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,   // 左下
            0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f   // 上
    };

    // 创建并配置VAO和VBO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
//    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
//    glBufferData(GL_ARRAY_BUFFER, sizeof(twoVertices), twoVertices, GL_STATIC_DRAW);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colorVertices), colorVertices, GL_STATIC_DRAW);

//    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
//    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    // 解绑
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    LOGI("Triangle data prepared");
}

void Triangle::draw() {
//    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDrawArrays(GL_TRIANGLES, 0 ,3);

    // 解绑
    glBindVertexArray(0);
}

const char *Triangle::getVertexShaderSource() const {
    return R"(#version 300 es
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
uniform float xOffset;
out vec3 ourColor;
void main() {
//    gl_Position = vec4(aPos, 1.0);
    gl_Position = vec4(aPos.x + xOffset, -aPos.y, aPos.z, 1.0);
    ourColor = aColor;
}
)";
}

const char *Triangle::getFragmentShaderSource() const {
    return R"(#version 300 es
precision mediump float;
in vec3 ourColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(ourColor, 1.0f);
}
)";
}

void Triangle::setUniform(GLuint program) const {
    glUniform1f(glGetUniformLocation(program, "xOffset"), 0.5);
}
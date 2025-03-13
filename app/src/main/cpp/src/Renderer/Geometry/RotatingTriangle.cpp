//
// Created by Weichuandong on 2025/3/13.
//
#include "Renderer/Geometry/RotatingTriangle.h"

void RotatingTriangle::init() {
    // 三角形顶点数据
    float vertices[] = {
            // 位置(x,y,z)                    // 颜色(r,g,b)
            0.0f,  0.5f, 0.0f,    1.0f, 0.0f, 0.0f,      // 顶点1（红色）
            -0.5f, -0.5f, 0.0f,    0.0f, 1.0f, 0.0f,    // 顶点2（绿色）
            0.5f, -0.5f, 0.0f,    0.0f, 0.0f, 1.0f   // 顶点3（蓝色）
    };

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)sizeof(float));
    glEnableVertexAttribArray(1);

    // 解绑
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // 计时器用于旋转
    mLastTime = std::chrono::high_resolution_clock::now();
}

void RotatingTriangle::draw() {
    glBindVertexArray(vao);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindVertexArray(0);
}

const char *RotatingTriangle::getVertexShaderSource() const {
    return R"(#version 300 es
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aColor;
out vec3 vColor;

uniform mat4 uModelMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;

void main() {
    // 将顶点位置通过MVP矩阵变换
    gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * vec4(aPosition, 1.0);
    vColor = aColor;
}
)";
}

const char *RotatingTriangle::getFragmentShaderSource() const {
    return R"(#version 300 es
precision mediump float;
in vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor, 1.0);
}
)";
}

void RotatingTriangle::setUniform(GLuint program) {
    // 设置视图和投影矩阵（通常只需设置一次）
    glm::mat4 viewMatrix = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 3.0f),    // 摄像机位置
            glm::vec3(0.0f, 0.0f, 0.0f),  // 目标位置
            glm::vec3(0.0f, 1.0f, 0.0f)      // 上向量
    );

    glm::mat4 projectionMatrix = glm::perspective(
            glm::radians(45.0f),          // 视野角度
            (float)9 / (float)16, // 宽高比
            0.1f, 100.0f                   // 近平面和远平面
    );

    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - mLastTime).count();

    // 更新模型矩阵以实现旋转
    auto modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::rotate(modelMatrix, deltaTime, glm::vec3(0.0f, 0.0f, 1.0f)); // 绕Z轴旋转

    // 设置MVP矩阵
    glUniformMatrix4fv(glGetUniformLocation(program, "uModelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(program, "uViewMatrix"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(glGetUniformLocation(program, "uProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
}

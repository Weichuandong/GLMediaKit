//
// Created by Weichuandong on 2025/3/10.
//
#include "Renderer/ShaderManager.h"

ShaderManager::ShaderManager() : program(0) {

}

ShaderManager::~ShaderManager() {
    cleanup();
}

GLuint ShaderManager::createShader(GLenum type, const char *shaderSource) {
    GLuint shader = glCreateShader(type);
    if (shader == 0) {
        LOGE("Failed to create shader");
        return 0;
    }

    glShaderSource(shader, 1, &shaderSource, NULL);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        LOGE("Error compiling shader: %s", infoLog);

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint ShaderManager::createProgram(const char* vsSource, const char* fgSource) {
    // 清理之前的program
    cleanup();

    GLuint vsShader = createShader(GL_VERTEX_SHADER, vsSource);
    GLuint fgShader = createShader(GL_FRAGMENT_SHADER, fgSource);

    if (vsShader == 0) {
        LOGE("Failed to create vsShader");
        return -1;
    }
    if (fgShader == 0) {
        LOGE("Failed to create fgShader");
        return -1;
    }

    program = glCreateProgram();
    if (program == 0) {
        LOGE("Failed to create program");
        return -1;
    }
    glAttachShader(program, vsShader);
    glAttachShader(program, fgShader);

    glLinkProgram(program);

    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        LOGE("Error linking program: %s", infoLog);

        glDeleteProgram(program);
        return -2;
    }

    glDeleteShader(vsShader);
    glDeleteShader(fgShader);

    return program;
}

void ShaderManager::cleanup() {
    if (program) {
        glDeleteProgram(program);
    }
}

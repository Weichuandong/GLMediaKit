//
// Created by Weichuandong on 2025/3/10.
//

#ifndef LEARNOPENGLES_SHADERMANAGER_H
#define LEARNOPENGLES_SHADERMANAGER_H

#include <GLES3/gl3.h>
#include <android/log.h>

#define LOG_TAG "ShaderManager"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class ShaderManager {
public:
    ShaderManager();
    ~ShaderManager();

    GLuint createShader(GLenum type, const char* shaderSource);

    GLuint createProgram(const char* vsSource, const char* fgSource);

    void cleanup();

private:
    GLuint program;

};
#endif //LEARNOPENGLES_SHADERMANAGER_H

//
// Created by Weichuandong on 2025/3/10.
//

#ifndef LEARNOPENGLES_GEOMETRY_H
#define LEARNOPENGLES_GEOMETRY_H

#include <GLES3/gl3.h>
#include <android/log.h>

#define LOG_TAG "Geometry"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


class Geometry {
public:
    Geometry();

    virtual ~Geometry();

    virtual void init();

    virtual void bind();

    virtual void draw();

    virtual void cleanup();

    virtual const char* getVertexShaderSource() const = 0;

    virtual const char* getFragmentShaderSource() const = 0;

protected:
    GLuint vao;
    GLuint vbo;
};

#endif //LEARNOPENGLES_GEOMETRY_H

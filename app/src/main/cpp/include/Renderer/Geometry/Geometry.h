//
// Created by Weichuandong on 2025/3/10.
//

#ifndef GLMEDIAKIT_GEOMETRY_H
#define GLMEDIAKIT_GEOMETRY_H

#include <GLES3/gl3.h>
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "Geometry", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "Geometry", __VA_ARGS__)


class Geometry {
public:
    Geometry();

    virtual ~Geometry();

    virtual void init();

    virtual void use(GLuint program);

    virtual void bind();

    virtual void draw();

    virtual void cleanup();

    virtual const char* getVertexShaderSource() const = 0;

    virtual const char* getFragmentShaderSource() const = 0;

    virtual void setUniform(GLuint program) = 0;

protected:
    GLuint vao;
    GLuint vbo;
};

#endif //GLMEDIAKIT_GEOMETRY_H

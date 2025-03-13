//
// Created by Weichuandong on 2025/3/13.
//

#ifndef GLMEDIAKIT_ROTATINGTRIANGLE_H
#define GLMEDIAKIT_ROTATINGTRIANGLE_H

#include "Renderer/Geometry/Geometry.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <chrono>

class RotatingTriangle : public Geometry {
public:
    void init() override;

    void draw() override;

    const char * getVertexShaderSource() const override;

    const char * getFragmentShaderSource() const override;

    void setUniform(GLuint program) override;

private:
    int mWidth;
    int mHeight;

    std::chrono::time_point<std::chrono::high_resolution_clock> mLastTime;
};
#endif //GLMEDIAKIT_ROTATINGTRIANGLE_H

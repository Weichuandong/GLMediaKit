//
// Created by Weichuandong on 2025/3/13.
//

#ifndef GLMEDIAKIT_MOVINGTRIANGLE_H
#define GLMEDIAKIT_MOVINGTRIANGLE_H

#include "Geometry.h"
#include "chrono"
#include "cmath"

class MovingTriangle : public Geometry {
public:

    void init() override;

    void draw() override;

    const char * getFragmentShaderSource() const override;

    const char * getVertexShaderSource() const override;

    void setUniform(GLuint program) override;

private:
    float mTranslationX = 0.0f;
    float mTranslationY = 0.0f;

    float rOffset = 0.0f;
    float gOffset = 0.0f;
    float bOffset = 0.0f;

    float mSpeedX = 0.15f;
    float mSpeedY = 0.1f;
    float mColorR = 0.1f;
    float mColorG = 0.1f;
    float mColorB = 0.1f;

    std::chrono::time_point<std::chrono::high_resolution_clock> mLastTime;
};

#endif //GLMEDIAKIT_MOVINGTRIANGLE_H

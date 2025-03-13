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
    float mSpeedX = 0.5f;
    float mSpeedY = 0.3f;

    std::chrono::time_point<std::chrono::high_resolution_clock> mLastTime;
    float mTotalTime = 0.0f;  // 追踪程序运行的总时间
};

#endif //GLMEDIAKIT_MOVINGTRIANGLE_H

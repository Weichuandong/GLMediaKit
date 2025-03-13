//
// Created by Weichuandong on 2025/3/10.
//

#ifndef GLMEDIAKIT_TRIANGLE_H
#define GLMEDIAKIT_TRIANGLE_H

#include "Geometry.h"

class Triangle : public Geometry {
public:
    void init() override;

    void draw() override;

    const char * getFragmentShaderSource() const override;

    const char * getVertexShaderSource() const override;

    void setUniform(GLuint program) const override;
};


#endif //GLMEDIAKIT_TRIANGLE_H

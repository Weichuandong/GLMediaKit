//
// Created by Weichuandong on 2025/3/13.
//

#ifndef GLMEDIAKIT_SQUARE_H
#define GLMEDIAKIT_SQUARE_H

#include "Geometry.h"

class Square : public Geometry {
public:

    void init() override;

    void draw() override;

    const char* getVertexShaderSource() const override;

    const char* getFragmentShaderSource() const override;

    void setUniform(GLuint program) override;

private:
    GLuint ebo;
};
#endif //GLMEDIAKIT_SQUARE_H

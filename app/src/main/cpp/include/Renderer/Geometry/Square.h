//
// Created by Weichuandong on 2025/3/13.
//

#ifndef GLMEDIAKIT_SQUARE_H
#define GLMEDIAKIT_SQUARE_H

#include "Geometry.h"

class Square : public Geometry{
public:
    Square() = default;

    void init() override;

    void bind() override;

    void draw() override;

    void cleanup() override;

    const char* getVertexShaderSource() const override;

    const char* getFragmentShaderSource() const override;

private:
    GLuint ebo;
};
#endif //GLMEDIAKIT_SQUARE_H

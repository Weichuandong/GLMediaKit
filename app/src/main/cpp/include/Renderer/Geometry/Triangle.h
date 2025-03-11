//
// Created by Weichuandong on 2025/3/10.
//

#ifndef LEARNOPENGLES_TRIANGLE_H
#define LEARNOPENGLES_TRIANGLE_H

#include "Geometry.h"

class Triangle : public Geometry {
public:
    void init() override;

    void bind() override;

    void draw() override;

    const char * getFragmentShaderSource() const override;

    const char * getVertexShaderSource() const override;
};


#endif //LEARNOPENGLES_TRIANGLE_H

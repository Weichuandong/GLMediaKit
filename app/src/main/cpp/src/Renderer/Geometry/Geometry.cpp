//
// Created by Weichuandong on 2025/3/10.
//
#include "Renderer/Geometry/Geometry.h"

Geometry::Geometry() :
          vao(0),
          vbo(0) {

}

Geometry::~Geometry() {
    cleanup();
}

void Geometry::cleanup() {
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
}

void Geometry::init() {

}

void Geometry::bind() {
    glBindVertexArray(vao);
}

void Geometry::draw() {

}

void Geometry::use(GLuint program) {
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(program);
}





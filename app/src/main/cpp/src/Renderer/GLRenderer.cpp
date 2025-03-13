//
// Created by Weichuandong on 2025/3/8.
//
#include "Renderer/GLRenderer.h"

#include <utility>

GLRenderer::GLRenderer() :
            mWidth(0),
            mHeight(0),
            shaderManager(std::make_unique<ShaderManager>()){
}

bool GLRenderer::init() {
//    geometry = std::make_shared<Triangle>();
//    geometry = std::make_shared<Square>();
//    geometry = std::make_shared<MovingTriangle>();
    geometry = std::make_shared<RotatingTriangle>();

    geometry->init();

    program = shaderManager->createProgram(
            geometry->getVertexShaderSource(),
            geometry->getFragmentShaderSource()
            );

    if (program == 0) {
        LOGE("Failed to create shader program");
        return false;
    }

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    return true;
}

void  GLRenderer::onSurfaceChanged(int width, int height) {
    mWidth = width;
    mHeight = height;

    glViewport(0.0f, 0.0f, mWidth, mHeight);
}

void GLRenderer::onDrawFrame() {
    if (!geometry || program == 0) {
        LOGE("Cannot render: geometry or program not initialized");
        return;
    }

    geometry->use(program);
    geometry->setUniform(program);
    geometry->bind();
    geometry->draw();
}

bool GLRenderer::setGeometry(const std::shared_ptr<Geometry>& geometry_) {
    if (!geometry_) {
        LOGE("Null geometry provided");
        return false;
    }

    geometry = geometry_;

    geometry->init();

    program = shaderManager->createProgram(
            geometry->getVertexShaderSource(),
            geometry->getFragmentShaderSource()
            );

    if (program == 0) {
        LOGE("Failed to create shader program for new geometry");
        return false;
    }

    return true;
}

void GLRenderer::release() {

}
//
// Created by Weichuandong on 2025/3/10.
//

#include "Renderer/ImageRenderer.h"


const char* vs_source = R"(#version 300 es
layout (location = 0) in vec4 aPosition;
layout (location = 1) in vec2 aTexCoord;
out vec2 vTexCoord;
void main() {
    gl_Position = aPosition;
    vTexCoord = aTexCoord;
}
)";

const char* fg_source = R"(#version 300 es
precision mediump float;
in vec2 vTexCoord;
uniform sampler2D uTexture;
out vec4 fragColor;
void main() {
    fragColor = texture(uTexture, vTexCoord);
}
)";

ImageRenderer::ImageRenderer():
          width(0),
          height(0),
          imageBuffer(nullptr),
          imageWidth(0),
          imageHeight(0),
          hasNewImage(false),
          textureId(0),
          programId(0){
}

ImageRenderer::~ImageRenderer() {
    release();
}

bool ImageRenderer::init() {
    programId = loadShaders();
    if (programId == 0) {
        LOGE("Failed to create shader program");
        return false;
    }

    // 创建纹理
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

void ImageRenderer::onSurfaceChanged(int w, int h) {
    LOGI("ImageRenderer::onSurfaceChanged - width: %d, height: %d, imageBuffer: %p", w, h, imageBuffer);
    width = w;
    height = h;

    glViewport(0, 0, width, height);
    if (!imageBuffer) {
        // 如果没有图像数据，生成默认图像
        LOGI("onSurfaceChanged: width = %d, height = %d", width, height);
        generateDefaultImage();
        hasNewImage = true;
    }
}

void ImageRenderer::onDrawFrame() {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (imageBuffer) {
        if (hasNewImage) {
            glBindTexture(GL_TEXTURE_2D, textureId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, imageBuffer);
            hasNewImage = false;
        }

        glUseProgram(programId);

        // 定义全屏四边形的顶点坐标
        static const GLfloat vertices[] = {
                -1.0f, -1.0f,  // 左下
                1.0f, -1.0f,  // 右下
                -1.0f,  1.0f,  // 左上
                1.0f,  1.0f   // 右上
        };

        // 定义纹理坐标
        static const GLfloat texCoords[] = {
                0.0f, 0.0f,  // 左下
                1.0f, 0.0f,  // 右下
                0.0f, 1.0f,  // 左上
                1.0f, 1.0f   // 右上
        };

        // 设置顶点位置数据
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
        glEnableVertexAttribArray(0);

        // 设置纹理坐标数据
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, texCoords);
        glEnableVertexAttribArray(1);

        // 绑定纹理
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glUniform1i(glGetUniformLocation(programId, "uTexture"), 0);

        // 绘制两个三角形组成的矩形
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        // 清理
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
    }

}

void ImageRenderer::release() {
    LOGI("ImageRenderer::release - freeing imageBuffer: %p", imageBuffer);
    if (imageBuffer) {
        free(imageBuffer);
        imageBuffer = nullptr;
    }

    if (textureId != 0) {
        glDeleteTextures(1, &textureId);
        textureId = 0;
    }

    if (programId != 0) {
        glDeleteProgram(programId);
        programId = 0;
    }
}

void ImageRenderer::setImage(void* data, int w, int h) {
    LOGI("ImageRenderer::setImage - width: %d, height: %d, addr: %p", w, h, data);
    if (imageBuffer) {
        free(imageBuffer);
    }

    imageWidth = w;
    imageHeight = h;
    imageBuffer = malloc(w * h * 4);
    memcpy(imageBuffer, data, w * h * 4);
    hasNewImage = true;
}

void ImageRenderer::generateDefaultImage() {
    // 设置默认图像尺寸
    imageWidth = width;
    imageHeight = height;

    // 分配内存
    if (imageBuffer) {
        free(imageBuffer);
    }
    imageBuffer = malloc(imageWidth * imageHeight * 4);

    // 生成彩色方格图案
    unsigned char* buffer = (unsigned char*)imageBuffer;
    for (int y = 0; y < imageHeight; y++) {
        for (int x = 0; x < imageWidth; x++) {
            int pixelIndex = (y * imageWidth + x) * 4;

            if (x < imageWidth/2 && y < imageHeight/2) {
                // 红色 - 左上
                buffer[pixelIndex] = 255;   // R
                buffer[pixelIndex+1] = 0;   // G
                buffer[pixelIndex+2] = 0;   // B
                buffer[pixelIndex+3] = 255; // A
            } else if (x >= imageWidth/2 && y < imageHeight/2) {
                // 绿色 - 右上
                buffer[pixelIndex] = 0;     // R
                buffer[pixelIndex+1] = 255; // G
                buffer[pixelIndex+2] = 0;   // B
                buffer[pixelIndex+3] = 255; // A
            } else if (x < imageWidth/2 && y >= imageHeight/2) {
                // 蓝色 - 左下
                buffer[pixelIndex] = 0;     // R
                buffer[pixelIndex+1] = 0;   // G
                buffer[pixelIndex+2] = 255; // B
                buffer[pixelIndex+3] = 255; // A
            } else {
                // 黄色 - 右下
                buffer[pixelIndex] = 255;   // R
                buffer[pixelIndex+1] = 255; // G
                buffer[pixelIndex+2] = 0;   // B
                buffer[pixelIndex+3] = 255; // A
            }
        }
    }
}

GLuint ImageRenderer::loadShaders() {
    GLuint vsShader = glCreateShader(GL_VERTEX_SHADER);
    if (vsShader == 0) {
        LOGE("Failed to create Shader");
        return 0;
    }

    glShaderSource(vsShader, 1, &vs_source, nullptr);
    glCompileShader(vsShader);

    int success;
    glGetShaderiv(vsShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vsShader, 512, NULL, infoLog);
        LOGE("Error compiling shader: %s", infoLog);

        glDeleteShader(vsShader);
        return 0;
    }

    GLuint fgShader = glCreateShader(GL_FRAGMENT_SHADER);
    if (fgShader == 0) {
        LOGE("Failed to create Shader");
        return 0;
    }

    glShaderSource(fgShader, 1, &fg_source, nullptr);
    glCompileShader(fgShader);

    glGetShaderiv(fgShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fgShader, 512, NULL, infoLog);
        LOGE("Error compiling shader: %s", infoLog);

        glDeleteShader(fgShader);
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program == 0) {
        LOGE("Failed to create program");
        return -1;
    }
    glAttachShader(program, vsShader);
    glAttachShader(program, fgShader);
    glLinkProgram(program);


    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        LOGE("Error linking program: %s", infoLog);

        glDeleteProgram(program);
        return -2;
    }

    glDeleteShader(vsShader);
    glDeleteShader(fgShader);
    return program;
}
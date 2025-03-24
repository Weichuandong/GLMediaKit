//
// Created by Weichuandong on 2025/3/21.
//

#include "Renderer/VideoRenderer.h"

VideoRenderer::VideoRenderer(std::shared_ptr<SafeQueue<AVFrame*>> frameQueue) :
    videoFrameQueue(std::move(frameQueue)),
    mode(ScalingMode::FIT)
{
    LOGI("VideoRenderer::VideoRenderer()");
}

bool VideoRenderer::init() {
    // 创建program
    program = shaderManager.createProgram(
            getVertexShaderSource(),
            getFragmentShaderSource()
    );
    if (program == 0) {
        LOGE("Failed to create shader program");
    }

    // 数据准备, 位置，纹理
    float vertices[] = {
            // 位置       // 纹理坐标
            -1.0f,  1.0f, 0.0f, 0.0f, // 左上
            1.0f,  1.0f, 1.0f, 0.0f, // 右上
            -1.0f, -1.0f, 0.0f, 1.0f, // 左下
            1.0f, -1.0f, 1.0f, 1.0f  // 右下
    };

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 位置属性
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)nullptr);
    glEnableVertexAttribArray(0);
    // 纹理坐标属性
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 创建纹理
    create_textures();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    return true;
}

void VideoRenderer::onSurfaceChanged(int width, int height) {
    if (width == surfaceWidth && height == surfaceHeight) {
        return;
    }
    LOGI("VideoRender: surface changed from %d*%d to %d*%d",
         surfaceWidth, surfaceHeight, width, height);
    surfaceWidth = width;
    surfaceHeight = height;

    glViewport(0.0f, 0.0f, surfaceWidth, surfaceHeight);

//    offscreenRenderer.initialize(surfaceWidth, surfaceHeight);

    if (videoWidth > 0 && videoHeight > 0) {
        // 如果已知视频尺寸
        calculateDisplayGeometry();
    } else {
        geometryNeedsChange = true;
    }
}

void VideoRenderer::onDrawFrame() {
    if (program == 0) {
        LOGE("Cannot render: program not initialized");
        return;
    }

//    offscreenRenderer.beginRender();
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(program);

    // 更新纹理
    update_textures();

    glUniform1i(glGetUniformLocation(program, "y_tex"), 0);
    glUniform1i(glGetUniformLocation(program, "u_tex"), 1);
    glUniform1i(glGetUniformLocation(program, "v_tex"), 2);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void VideoRenderer::release() {

}

const char *VideoRenderer::getVertexShaderSource() const {
    return R"(#version 300 es
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";
}

const char *VideoRenderer::getFragmentShaderSource() const {
    return R"(#version 300 es
precision mediump float;
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D y_tex;
uniform sampler2D u_tex;
uniform sampler2D v_tex;

void main() {
    float y = texture(y_tex, TexCoord).r;
    float u = texture(u_tex, TexCoord).r - 0.5;
    float v = texture(v_tex, TexCoord).r - 0.5;

    vec3 rgb;
    rgb.r = y + 1.402 * v;
    rgb.g = y - 0.344136 * u - 0.714136 * v;
    rgb.b = y + 1.772 * u;

    rgb = clamp(rgb, 0.0, 1.0);

    FragColor = vec4(rgb, 1.0);
}
)";
}

void VideoRenderer::create_textures() {
    glGenTextures(1, &y_tex);
    glGenTextures(1, &u_tex);
    glGenTextures(1, &v_tex);

    // 配置y纹理
    glBindTexture(GL_TEXTURE_2D, y_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 配置uv纹理
    glBindTexture(GL_TEXTURE_2D, u_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, v_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void VideoRenderer::update_textures() {
    // 从队列中取出一帧frame
    AVFrame* frame;
    videoFrameQueue->pop(frame, 10);

    if (frame && frame->width > 0 && frame->height > 0) {
        // 视频尺寸变化
        if (frame->width != videoWidth || frame->height != videoHeight) {
            videoWidth = frame->width;
            videoHeight = frame->height;
            LOGI("Video size changed to %dx%d", videoWidth, videoHeight);
            LOGI("frame->width = %d, frame->height = %d, frame->linesize[0] = %d, frame->linesize[1] = %d",
                 frame->width, frame->height, frame->linesize[0], frame->linesize[1]);
            geometryNeedsChange = true;
        }

        // 如果几何形状需要更新且Surface尺寸已知
        if (geometryNeedsChange && surfaceWidth > 0 && surfaceHeight > 0) {
            calculateDisplayGeometry();
            geometryNeedsChange = false;
        }

        // 更新y纹理
        glPixelStorei(GL_UNPACK_ROW_LENGTH, frame->linesize[0]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, y_tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width, frame->height, 0,
                     GL_RED, GL_UNSIGNED_BYTE, frame->data[0]);

        // 更新u纹理
        glPixelStorei(GL_UNPACK_ROW_LENGTH, frame->linesize[1]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, u_tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width / 2, frame->height / 2, 0,
                     GL_RED, GL_UNSIGNED_BYTE, frame->data[1]);

        // 更新v纹理
        glActiveTexture(GL_TEXTURE2);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, frame->linesize[1]);
        glBindTexture(GL_TEXTURE_2D, v_tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width / 2, frame->height / 2, 0,
                     GL_RED, GL_UNSIGNED_BYTE, frame->data[2]);

        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        av_frame_free(&frame);
    }
}

VideoRenderer::~VideoRenderer() {
    offscreenRenderer.cleanup();

    glDeleteProgram(program);
    glDeleteTextures(1, &y_tex);
    glDeleteTextures(1, &u_tex);
    glDeleteTextures(1, &v_tex);

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

void VideoRenderer::calculateDisplayGeometry() {
    // 分配顶点和纹理坐标数组;
    float vertices[16] = {
            // 默认顶点坐标， 纹理坐标
            -1.0f, 1.0f, 0.0f, 0.0f,     //左上
            1.0f, 1.0f, 1.0f, 0.0f,      //右上
            -1.0f, -1.0f, 0.0f, 1.0f,   // 左下
            1.0f, -1.0f, 1.0f, 1.0f    // 右下
    };

    // 计算宽高比
    float videoRatio = (float)videoWidth / videoHeight;
    float surfaceRatio = (float)surfaceWidth / surfaceHeight;

    LOGI("Calculating geometry: video %dx%d (ratio %.2f), surface %dx%d (ratio %.2f), mode %d",
         videoWidth, videoHeight, videoRatio, surfaceWidth, surfaceHeight, surfaceRatio, (int)mode);

    if (mode == IRenderer::ScalingMode::FIT) {
        // 适应模式：保持比例，显示完整内容，可能有黑边
        if (videoRatio > surfaceRatio) {
            // 视频更宽，上下有黑边
            float scaledHeight = surfaceRatio / videoRatio;

            // 调整顶点Y坐标 (压缩高度)
            vertices[1] = scaledHeight;       // 左上y
            vertices[5] = scaledHeight;       // 右上y
            vertices[9] = -scaledHeight;      // 左下y
            vertices[13] = -scaledHeight;     // 右下y
        }
        else {
            // 视频更高，左右有黑边
            float scaledWidth = videoRatio / surfaceRatio;

            // 调整顶点X坐标 (压缩宽度)
            vertices[0] = -scaledWidth;      // 左上x
            vertices[4] = scaledWidth;       // 右上x
            vertices[8] = -scaledWidth;      // 左下x
            vertices[12] = scaledWidth;      // 右下x
        }
    }
    else if (mode == IRenderer::ScalingMode::FILL) {
        // 填充模式：保持比例，填满Surface，可能裁剪内容
        if (videoRatio > surfaceRatio) {
            // 视频更宽，需要裁剪左右
            float scale = videoRatio / surfaceRatio;
            float offset = (1.0f - 1.0f/scale) / 2.0f;

            // 调整纹理坐标 (裁剪宽度)
            vertices[2] = offset;           // 左上 s
            vertices[6] = 1.0f - offset;    // 右上 s
            vertices[10] = offset;          // 左下 s
            vertices[14] = 1.0f - offset;   // 右下 s
        }
        else {
            // 视频更高，需要裁剪上下
            float scale = surfaceRatio / videoRatio;
            float offset = (1.0f - 1.0f/scale) / 2.0f;

            // 调整纹理坐标 (裁剪高度)
            vertices[3] = offset;           // 左上 t
            vertices[7] = offset;           // 右上 t
            vertices[11] = 1.0f - offset;   // 左下 t
            vertices[15] = 1.0f - offset;   // 右下 t
        }
    }
    // STRETCH模式保持默认值 - 拉伸填满

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
}

void VideoRenderer::setScaleMode(IRenderer::ScalingMode newMode) {
    if (mode != newMode) {
        mode = newMode;
        LOGI("Scale mode changed to %d", (int)mode);

        // 如果有视频尺寸和Surface尺寸，立即重新计算几何形状
        if (videoWidth > 0 && videoHeight > 0 && surfaceWidth > 0 && surfaceHeight > 0) {
            calculateDisplayGeometry();
        } else {
            geometryNeedsChange = true;
        }
    }

}
//
// Created by Weichuandong on 2025/3/13.
//

#include "TextureManger.h"

TextureManager::TextureManager() {
}

TextureManager::~TextureManager() {
    deleteAllTextures();
}

GLuint TextureManager::createTextureFromBitmap(JNIEnv* env, jobject bitmap, std::string key) {
    // 获取bitmap信息
    AndroidBitmapInfo bitmapInfo;
    if (AndroidBitmap_getInfo(env, bitmap, &bitmapInfo) < 0) {
        LOGE("Failed to get bitmap info");
        return 0;
    }

    // 检查像素格式
    if (bitmapInfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGE("Bitmap format not supported (must be RGBA_8888)");
        return 0;
    }

    // 锁定像素缓冲区
    void* bitmapPixels;
    if (AndroidBitmap_lockPixels(env, bitmap, &bitmapPixels) < 0) {
        LOGE("Failed to lock bitmap pixels");
        return 0;
    }

    // 创建OpenGL纹理
    GLuint textureId;
    glGenTextures(1, &textureId);
    if (textureId == 0) {
        LOGE("Failed to generate texture");
        AndroidBitmap_unlockPixels(env, bitmap);
        return 0;
    }

    // 绑定纹理并设置参数
    glBindTexture(GL_TEXTURE_2D, textureId);
    setDefaultTextureParameters(textureId);

    // 上传纹理数据
    glTexImage2D(
            GL_TEXTURE_2D,          // 纹理格式
            0,                       // mipmap级别
            GL_RGBA,          // 内部格式
            bitmapInfo.width,
            bitmapInfo.height,
            0,                      // 边框
            GL_RGBA,                // 数据格式
            GL_UNSIGNED_BYTE,        // 数据类型
            bitmapPixels
    );

    // 解锁像素缓冲区
    AndroidBitmap_unlockPixels(env, bitmap);

    // 跟踪纹理ID便于管理
    allTextures.emplace_back(textureId);
    cacheTexture(key, textureId);

    LOGI("Created texture id: %d (%dx%d)", textureId, bitmapInfo.width, bitmapInfo.height);
    return textureId;
}

GLuint TextureManager::loadTexture(const std::string& path) {
    // 首先检查缓存
    auto it = textureCache.find(path);
    if (it != textureCache.end()) {
        return it->second;
    }

    // 实际项目中，这里需要通过资源管理器或JNI加载图像文件
    // 此处为简化示例，返回0表示未实现
    LOGI("Loading texture from path not implemented yet");
    return 0;
}

GLuint TextureManager::getTexture(const std::string& key) {
    auto it = textureCache.find(key);
    if (it != textureCache.end()) {
        return it->second;
    }
    return 0;
}

void TextureManager::cacheTexture(const std::string& key, GLuint textureId) {
    // 先移除旧纹理（如果存在）
    auto it = textureCache.find(key);
    if (it != textureCache.end()) {
        glDeleteTextures(1, &it->second);
    }

    // 缓存新纹理
    textureCache[key] = textureId;
}

void TextureManager::deleteTexture(const std::string& key) {
    auto it = textureCache.find(key);
    if (it != textureCache.end()) {
        GLuint texId = it->second;
        glDeleteTextures(1, &texId);

        // 从跟踪列表中移除
        auto texIt = std::find(allTextures.begin(), allTextures.end(), texId);
        if (texIt != allTextures.end()) {
            allTextures.erase(texIt);
        }

        textureCache.erase(it);
    }
}

void TextureManager::deleteTexture(GLuint textureId) {
    glDeleteTextures(1, &textureId);

    // 从跟踪列表中移除
    auto texIt = std::find(allTextures.begin(), allTextures.end(), textureId);
    if (texIt != allTextures.end()) {
        allTextures.erase(texIt);
    }

    // 从缓存中移除
    for (auto it = textureCache.begin(); it != textureCache.end(); ) {
        if (it->second == textureId) {
            it = textureCache.erase(it);
        } else {
            ++it;
        }
    }
}

void TextureManager::deleteAllTextures() {
    // 删除所有纹理
    for (GLuint texId : allTextures) {
        glDeleteTextures(1, &texId);
    }

    allTextures.clear();
    textureCache.clear();
}

void TextureManager::setDefaultTextureParameters(GLuint textureId) {
    glBindTexture(GL_TEXTURE_2D, textureId);

    // 设置纹理过滤方式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 设置纹理环绕方式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

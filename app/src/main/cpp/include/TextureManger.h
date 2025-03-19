//
// Created by Weichuandong on 2025/3/13.
//

#ifndef GLMEDIAKIT_TEXTUREMANGER_H
#define GLMEDIAKIT_TEXTUREMANGER_H

#include <jni.h>
#include <GLES3/gl3.h>
#include <string>
#include <unordered_map>
#include <android/log.h>
#include <android/bitmap.h>
#include <vector>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "TextureManager", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "TextureManager", __VA_ARGS__)

class TextureManager {
public:
    TextureManager();
    ~TextureManager();

    // 从Java Bitmap 创建纹理
    GLuint createTextureFromBitmap(JNIEnv* env, jobject bitmap, std::string key);

    // 从文件路径加载纹理
    GLuint loadTexture(const std::string& path);

    // 获取已缓存的纹理，如不存在则返回0
    GLuint getTexture(const std::string& key);

    // 缓存纹理
    void cacheTexture(const std::string& key, GLuint textureId);

    // 删除特定纹理
    void deleteTexture(const std::string& key);
    void deleteTexture(GLuint textureId);

    // 删除所有纹理
    void deleteAllTextures();

private:
    // 纹理缓存
    std::unordered_map<std::string, GLuint> textureCache;

    // 跟踪所有创建的纹理ID，便于清理
    std::vector<GLuint> allTextures;

    // 工具函数：配置纹理参数
    void setDefaultTextureParameters(GLuint textureId);

};

#endif //GLMEDIAKIT_TEXTUREMANGER_H

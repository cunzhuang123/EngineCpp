// ImageResource.cpp

#include "ImageResource.h"
#include <iostream>
#include <sstream>
#include <vector>

#include "../nanovg/stb_image.h"


ImageResource::ImageResource(const std::string& filePath)
    :filePath(filePath), width(0), height(0), textureId(0) {
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    // 初始化纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    std::cerr << "ImageResource textureId: " << textureId << std::endl;
}

ImageResource::~ImageResource() {
    cleanup();
}

bool ImageResource::initialize(int rotate) {
    // 加载图片数据
    int nrChannels;
    // 如果图片y轴方向翻转，避免纹理显示异常
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;
        else {
            std::cerr << "不支持的图片格式: " << nrChannels << " 通道" << std::endl;
            stbi_image_free(data);
            return false;
        }

        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        std::clog << "成功加载纹理: " << filePath << " (" << width << "x" << height << ", " << nrChannels << " 通道)" << std::endl;

        // 释放图片数据
        stbi_image_free(data);
        generateVertices();
        return true;
    }
    else {
        std::cerr << "加载纹理失败: " << filePath << std::endl;
        stbi_image_free(data);
        return false;
    }
}

void ImageResource::generateVertices() {
    // 定义四个顶点，以纹理的中心为原点
    float w = width / 2.0f;
    float h = height / 2.0f;

    // 顶点数据：x, y, z, u, v
    vertices = {
        -w,  h, 0.0f, 0.0f, 1.0f,  // 左上角
         w,  h, 0.0f, 1.0f, 1.0f,  // 右上角
        -w, -h, 0.0f, 0.0f, 0.0f,  // 左下角
         w, -h, 0.0f, 1.0f, 0.0f,  // 右下角
    };
}

const std::vector<float>& ImageResource::getVertices() const {
    return vertices;
}

GLuint ImageResource::getWidth() const {
    return width;
}

GLuint ImageResource::getHeight() const {
    return height;
}

GLuint ImageResource::getSourceWidth() const {
    return width;
}

GLuint ImageResource::getSourceHeight() const {
    return height;
}

void ImageResource::cleanup() {
    if (textureId != 0) {
        glDeleteTextures(1, &textureId);
        textureId = 0;
    }
}
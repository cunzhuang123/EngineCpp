

// VideoResource.h

#ifndef VIDEORESOURCE_H
#define VIDEORESOURCE_H

#include <glad/glad.h>
#include <string>
#include <vector>
#include "RendererResource.h"

// 添加 FFmpeg 头文件
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}


class VideoResource :public  RendererResource{
public:
    VideoResource(const std::string& filePath);
    virtual ~VideoResource();

    virtual bool initialize(int rotate);

    virtual GLuint getWidth() const override;
    virtual GLuint getHeight() const override;
    virtual GLuint getSourceWidth() const override;
    virtual GLuint getSourceHeight() const override;

    virtual const std::vector<float>& getVertices() const override;
    virtual GLuint getTexture() override {return texture;};


    int getRotation() const { return rotation; }


    double getDuration() const;
    bool getFrameAt(double time);
    void destroy();

private:
    int normalizeRotation(int degrees);
    void generateVertices(int rotate);
    void updateTexture(uint8_t* data, int width, int height);
    std::string filePath;
    int videoStreamIndex;

    // FFmpeg 相关成员
    AVFormatContext* formatContext;
    AVCodecContext* codecContext;
    AVFrame* avFrame;
    AVPacket* avPacket;
    struct SwsContext* swsContext;
    GLuint texture;

    GLuint width;
    GLuint height;
    double duration;

    // int sourceWidth;
    // int sourceHeight;

    int rotation;
    std::vector<float> vertices; // 存储顶点数据的成员变量

    // 缓存 RGB 数据的缓冲区
    uint8_t* rgbBuffer;
    int rgbBufferSize;
    double preTime;
};

#endif // VIDEORESOURCE_H
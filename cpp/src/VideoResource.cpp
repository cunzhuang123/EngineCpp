

// VideoResource.cpp

#include "VideoResource.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/version.h>
}

VideoResource::VideoResource(const std::string& filePath)
    : filePath(filePath), formatContext(nullptr), codecContext(nullptr),
      videoStreamIndex(-1), avFrame(nullptr), avPacket(nullptr), swsContext(nullptr),
      width(0), height(0), duration(0.0), rgbBuffer(nullptr), rgbBufferSize(0) {

    preTime = -1.0;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // 初始化纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

VideoResource::~VideoResource() {
    destroy();
}

bool VideoResource::initialize(int rotate) {
    // 打开输入文件
    if (avformat_open_input(&formatContext, filePath.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "无法打开输入文件：" << filePath << std::endl;
        return false;
    }

    // 检索流信息
    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        std::cerr << "无法获取流信息：" << filePath << std::endl;
        return false;
    }

    // 查找第一个视频流
    for (unsigned int i = 0; i < formatContext->nb_streams; ++i) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = static_cast<int>(i);
            break;
        }
    }

    if (videoStreamIndex == -1) {
        std::cerr << "找不到视频流：" << filePath << std::endl;
        return false;
    }

    // 获取视频流的编解码器参数
    AVCodecParameters* codecParameters = formatContext->streams[videoStreamIndex]->codecpar;

    // 查找解码器
    const AVCodec* codec = avcodec_find_decoder(codecParameters->codec_id);
    if (!codec) {
        std::cerr << "找不到合适的解码器：" << filePath << std::endl;
        return false;
    }

    // 创建解码器上下文
    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        std::cerr << "无法分配解码器上下文：" << filePath << std::endl;
        return false;
    }

    // 从编解码器参数复制参数到解码器上下文
    if (avcodec_parameters_to_context(codecContext, codecParameters) < 0) {
        std::cerr << "无法复制编解码器参数：" << filePath << std::endl;
        return false;
    }

    // 设置解码器的线程数
    codecContext->thread_count = std::thread::hardware_concurrency(); // 或者设定一个固定值，例如 4

    // 如果无法获取硬件并发数，设置一个默认值
    if (codecContext->thread_count == 0) {
        codecContext->thread_count = 4; // 默认使用 4 个线程
    }
    // 如果无法获取硬件并发数，设置一个默认值
    if (codecContext->thread_count > 16) {
        codecContext->thread_count = 16; // 默认使用 4 个线程
    }
    codecContext->thread_type = FF_THREAD_SLICE;    // 关键点：使用slice多线程

    // 在 avcodec_open2 之前加上这些容错设置
    codecContext->err_recognition = AV_EF_IGNORE_ERR | AV_EF_CAREFUL;
    codecContext->flags |= AV_CODEC_FLAG_OUTPUT_CORRUPT; // 允许输出损坏帧
    codecContext->flags2 |= AV_CODEC_FLAG2_SHOW_ALL;     // 尽可能多地输出有效帧

    // 打开解码器
    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        std::cerr << "无法打开解码器：" << filePath << std::endl;
        return false;
    }

    // 分配帧和包
    avFrame = av_frame_alloc();
    avPacket = av_packet_alloc();

    if (!avFrame || !avPacket) {
        std::cerr << "无法分配帧或包：" << filePath << std::endl;
        return false;
    }

    // 获取视频宽高
    width = codecContext->width;
    height = codecContext->height;

    // 获取视频时长（秒）
    if (formatContext->duration != AV_NOPTS_VALUE)
        duration = formatContext->duration / (double)AV_TIME_BASE;
    else
        duration = 0.0;

    // 初始化阶段
    swsContext = sws_getContext(
        codecContext->width,
        codecContext->height,
        codecContext->pix_fmt,
        codecContext->width,
        codecContext->height,
        AV_PIX_FMT_RGB24,
        SWS_FAST_BILINEAR, // 高效选项
        nullptr,
        nullptr,
        nullptr
    );
    if (!swsContext) {
        std::cerr << "无法初始化 swsContext" << std::endl;
        return false;
    }

    // 你的原始initialize()函数代码不变，后面增加一次seek到开头即可：
    int64_t timestamp = av_rescale_q(0, AV_TIME_BASE_Q, formatContext->streams[videoStreamIndex]->time_base);
    av_seek_frame(formatContext, videoStreamIndex, timestamp, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(codecContext);
    preTime = -1.0; // 初始化标记为还未解码任何帧
    
    // 分配 RGB 图像数据缓冲区
    rgbBufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
    rgbBuffer = (uint8_t*)av_malloc(rgbBufferSize * sizeof(uint8_t));
    if (!rgbBuffer) {
        std::cerr << "无法分配 RGB 图像数据缓冲区" << std::endl;
        return false;
    }

    // 生成顶点数据
    generateVertices(rotate);

    return true;
}


bool VideoResource::getFrameAt(double time) {
    const double tolerance = 0.033;

    if (!formatContext || !codecContext)
        return false;
    if (std::abs(preTime - time) <= tolerance)
    {
        return true;
    }

    preTime = time;

    bool frameFound = false;
    bool eof_reached = false;

    while (true) {
        int ret = av_read_frame(formatContext, avPacket);
        if (ret < 0) {
            // 文件读取结束，发送空包到解码器，取出剩余的缓存帧
            avcodec_send_packet(codecContext, nullptr);
            eof_reached = true;
        } else if (avPacket->stream_index == videoStreamIndex) {
            avcodec_send_packet(codecContext, avPacket);
            av_packet_unref(avPacket);
        } else {
            av_packet_unref(avPacket);
            continue;
        }

        while (avcodec_receive_frame(codecContext, avFrame) == 0) {
            double frameTime = avFrame->best_effort_timestamp * av_q2d(formatContext->streams[videoStreamIndex]->time_base);
            uint8_t* dest[4] = { rgbBuffer, nullptr, nullptr, nullptr };
            int destLinesize[4] = { static_cast<int>(width) * 3, 0, 0, 0 };
            sws_scale(swsContext, avFrame->data, avFrame->linesize, 0, height, dest, destLinesize);

            if (frameTime >= time - tolerance) {
                updateTexture(rgbBuffer, width, height);
                av_frame_unref(avFrame);
                return true;
            }
            av_frame_unref(avFrame);
        }

        if (eof_reached) break; // EOF后解码器缓存帧全部取出，结束循环
    }

    std::cerr << "[警告] 已到达视频末尾，未能找到精确的帧，但已使用最后解码的帧:" << time << " 秒。" << std::endl;
    return true;
}



int VideoResource::normalizeRotation(int degrees) {
    return ((degrees % 360) + 360) % 360;
}

void VideoResource::generateVertices(int rotation) {
    rotation = normalizeRotation(rotation);
    this->rotation = rotation;

    float w = width / 2.0f;
    float h = height / 2.0f;

    vertices = {
        -w,  h, 0.0f, 0.0f, 0.0f,
            w,  h, 0.0f, 1.0f, 0.0f,
        -w, -h, 0.0f, 0.0f, 1.0f,
            w, -h, 0.0f, 1.0f, 1.0f,
    };
}

void VideoResource::updateTexture(uint8_t* data, int width, int height) {
    if (!data)
        return;

    // 将 RGB 数据上传到 OpenGL 纹理
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        width,
        height,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        data
    );
}


void VideoResource::destroy() {
    if (avFrame) {
        av_frame_free(&avFrame);
        avFrame = nullptr;
    }
    if (avPacket) {
        av_packet_free(&avPacket);
        avPacket = nullptr;
    }
    if (codecContext) {
        avcodec_free_context(&codecContext);
        codecContext = nullptr;
    }
    if (formatContext) {
        avformat_close_input(&formatContext);
        formatContext = nullptr;
    }
    if (swsContext) {
        sws_freeContext(swsContext);
        swsContext = nullptr;
    }
    if (rgbBuffer) {
        av_free(rgbBuffer);
        rgbBuffer = nullptr;
        rgbBufferSize = 0;
    }
}

GLuint VideoResource::getWidth() const {
    if (rotation == 90 || rotation == 270)
    {
        return height;
    }
    else
    {
        return width;
    }
}

GLuint VideoResource::getHeight() const {
    if (rotation == 90 || rotation == 270)
    {
        return width;
    }
    else
    {
        return height;
    }
}

GLuint VideoResource::getSourceWidth() const {
    return width;
}

GLuint VideoResource::getSourceHeight() const {
    return height;
}

double VideoResource::getDuration() const {
    return duration;
}

const std::vector<float>& VideoResource::getVertices() const {
    return vertices;  // 返回顶点数据的常量引用，避免拷贝
}
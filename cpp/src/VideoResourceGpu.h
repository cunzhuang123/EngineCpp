// VideoResource.h
#ifndef VIDEORESOURCE_H
#define VIDEORESOURCE_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <iostream>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <chrono>
#include <atomic>

class VideoResource {
public:
    // 构造函数
    VideoResource(const std::string& filePath);

    // 析构函数
    ~VideoResource();

    // 初始化方法
    bool initialize();

    // 获取指定时间点的帧
    bool getFrameAt(double time, std::function<void(uint8_t* data, int width, int height)> callback);

    // 清理资源
    void destroy();

    // 获取视频属性
    int getWidth() const;
    int getHeight() const;
    double getDuration() const;
    const std::vector<float>& getVertices() const;

private:
    // 内部解码器类，仅由解码线程使用
    class FFmpegInstance {
    public:
        FFmpegInstance();
        ~FFmpegInstance();

        bool open(const std::string& filename, int stream_index);
        void close();
        int getFrame(AVFrame* frame);
        const AVStream* getStream() const { return avstream_; }
        AVFormatContext* getFormatContext() const { return fmt_ctx_; }
        AVCodecContext* getCodecContext() const { return codec_ctx_; }

        void seek(int64_t timestamp);

    private:
        AVFormatContext* fmt_ctx_;
        AVCodecContext* codec_ctx_;
        AVStream* avstream_;
        AVDictionary* opts_;
    };

    // 帧缓存机制
    std::deque<AVFrame*> frameCache_;
    const size_t maxCacheSize_ = 30; // 根据需要调整缓存大小
    std::mutex resourceMutex_; // 保护所有共享资源
    std::condition_variable cacheCondVar_; // 条件变量用于同步

    // 解码线程相关
    std::thread decoderThread_;
    std::atomic<bool> stopDecoder_;
    std::atomic<bool> decoderReady_;

    // Seek请求相关
    bool seekRequested_;
    int64_t seekTimestamp_;

    // FFmpeg相关
    std::string filePath;
    FFmpegInstance decoder_;
    int videoStreamIndex;
    double duration;

    // 解码相关
    AVFrame* avFrame;
    AVPacket* avPacket;
    SwsContext* swsContext;
    uint8_t* rgbBuffer;
    int rgbBufferSize;
    int width;
    int height;

    // 顶点数据
    std::vector<float> vertices;

    // 私有方法
    void generateVertices();
    void decodingLoop();
    void clearCache();
};



#endif // VIDEORESOURCE_H
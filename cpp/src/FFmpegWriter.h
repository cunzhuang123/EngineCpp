#ifndef FFMPEGWRITER_H
#define FFMPEGWRITER_H

// FFmpegWriter.h
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <memory>
#include <string>

// 定义FrameData结构体
struct FrameData {
    std::vector<uint8_t> data;
    int width;
    int height;
};

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
    #include <libavutil/opt.h>
    #include <libavformat/avformat.h>
}

class FFmpegWriter {
public:
    FFmpegWriter(const std::string& filename, int width, int height, int fps, int kbps);
    ~FFmpegWriter();
    
    bool initialize(const std::string& filename);
    bool pushFrame(uint8_t* rgbData, int width, int height);
    void startEncoding();
    void stopEncoding();
    void finalize();

private:
    void encodingLoop();
    bool encodeFrame(const uint8_t* rgbData);

    // FFmpeg相关成员
    AVFormatContext* formatContext;
    AVStream* videoStream;
    AVCodecContext* codecContext;
    SwsContext* swsContext;
    AVFrame* frame;
    AVPacket* packet;

    // 多线程编码
    std::thread encodingThread;
    std::mutex queueMutex;
    std::condition_variable queueCV;
    std::queue<FrameData> frameQueue;
    bool isEncoding;

    // 索引和缓冲区
    int frameIndex;
    int width;
    int height;
    int fps;
    int mBitRate;
};

#endif // 
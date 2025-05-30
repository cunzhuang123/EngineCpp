// FFmpegWriter.cpp
#include "FFmpegWriter.h"
#include <iostream>
#include <cstring>


FFmpegWriter::FFmpegWriter(const std::string& filename, int width, int height, int fps, int mBitRate)
    : formatContext(nullptr), videoStream(nullptr), codecContext(nullptr),
      swsContext(nullptr), frame(nullptr), packet(nullptr),
      frameIndex(0), width(width), height(height), fps(fps), mBitRate(mBitRate), isEncoding(false) 
{
    // 构造函数不再调用 initialize
}

FFmpegWriter::~FFmpegWriter() {
    // finalize();
}

bool FFmpegWriter::initialize(const std::string& filename) {

    // 分配输出上下文
    if (avformat_alloc_output_context2(&formatContext, nullptr, nullptr, filename.c_str()) < 0) {
        std::cerr << "无法分配输出上下文" << std::endl;
        return false;
    }

    // 查找H.264编码器
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        std::cerr << "找不到 H.264 编码器" << std::endl;
        return false;
    }

    // 创建新的视频流
    videoStream = avformat_new_stream(formatContext, codec);
    if (!videoStream) {
        std::cerr << "无法创建新的视频流" << std::endl;
        return false;
    }
    videoStream->id = formatContext->nb_streams - 1;

    // 分配编解码器上下文
    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        std::cerr << "无法分配编解码器上下文" << std::endl;
        return false;
    }

    // 设置编码器参数
    codecContext->codec_id = AV_CODEC_ID_H264;
    codecContext->bit_rate = mBitRate*1000000;//码率kbpsM
    codecContext->width = width;
    codecContext->height = height;
    codecContext->time_base = AVRational{1, fps};
    codecContext->framerate = AVRational{fps, 1};
    codecContext->gop_size = 10;
    codecContext->max_b_frames = 0; // 禁用B帧
    codecContext->pix_fmt = AV_PIX_FMT_YUV420P;

    // 设置H.264预设
    if (codecContext->codec_id == AV_CODEC_ID_H264) {
        av_opt_set(codecContext->priv_data, "preset", "ultrafast", 0); // 使用ultrafast预设
    }

    // 设置全局头
    if (formatContext->oformat->flags & AVFMT_GLOBALHEADER) {
        codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    // 打开编码器
    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        std::cerr << "无法打开编解码器" << std::endl;
        return false;
    }

    // 复制编解码器参数到流
    if (avcodec_parameters_from_context(videoStream->codecpar, codecContext) < 0) {
        std::cerr << "无法复制编解码器参数到流" << std::endl;
        return false;
    }

    // 设置流的时间基
    videoStream->time_base = codecContext->time_base;
    videoStream->avg_frame_rate = codecContext->framerate;
    videoStream->r_frame_rate = codecContext->framerate;

    // 打印格式信息
    av_dump_format(formatContext, 0, filename.c_str(), 1);

    // 打开输出文件
    if (!(formatContext->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&formatContext->pb, filename.c_str(), AVIO_FLAG_WRITE) < 0) {
            std::cerr << "无法打开输出文件: " << filename << std::endl;
            return false;
        }
    }

    // 写入文件头部
    if (avformat_write_header(formatContext, nullptr) < 0) {
        std::cerr << "无法写入文件头部" << std::endl;
        return false;
    }

    // 初始化swsContext
    swsContext = sws_getContext(
        width,
        height,
        AV_PIX_FMT_RGB24,
        width,
        height,
        AV_PIX_FMT_YUV420P,
        SWS_FAST_BILINEAR,
        nullptr,
        nullptr,
        nullptr
    );

    if (!swsContext) {
        std::cerr << "无法初始化 swsContext" << std::endl;
        return false;
    }

    // 分配帧
    frame = av_frame_alloc();
    if (!frame) {
        std::cerr << "无法分配 AVFrame" << std::endl;
        return false;
    }
    frame->format = codecContext->pix_fmt;
    frame->width  = codecContext->width;
    frame->height = codecContext->height;

    if (av_frame_get_buffer(frame, 32) < 0) {
        std::cerr << "无法分配帧缓冲区" << std::endl;
        return false;
    }

    // 分配包
    packet = av_packet_alloc();
    if (!packet) {
        std::cerr << "无法分配 AVPacket" << std::endl;
        return false;
    }

    return true;
}

bool FFmpegWriter::encodeFrame(const uint8_t* rgbData) {
    if (rgbData) {
        // frame->pts = frameIndex++;
        // std::cerr << "Encoding frame " << frameIndex << " with PTS " << frame->pts << std::endl;

        // // RGB到YUV的颜色空间转换
        // uint8_t* srcSlices[1] = { const_cast<uint8_t*>(rgbData) };
        // int srcStrides[1] = { 3 * width };
        // sws_scale(
        //     swsContext,
        //     srcSlices,
        //     srcStrides,
        //     0,
        //     height,
        //     frame->data,
        //     frame->linesize
        // );

        frame->pts = frameIndex++;
        // std::cerr << "Encoding frame " << frameIndex << " with PTS " << frame->pts << std::endl;

        // RGB到YUV的颜色空间转换
        int srcStride = 3 * width;
        uint8_t* srcSlices[1];
        int srcStrides[1];

        // 设置负的步幅，实现垂直翻转
        srcStrides[0] = -srcStride;
        // 指向最后一行的起始地址
        srcSlices[0] = const_cast<uint8_t*>(rgbData) + srcStride * (height - 1);

        sws_scale(
            swsContext,
            srcSlices,
            srcStrides,
            0,
            height,
            frame->data,
            frame->linesize
        );

    } else {
        // 发送NULL帧以刷新编码器
        std::cerr << "Flushing encoder" << std::endl;
    }

    // 发送帧到编码器
    if (avcodec_send_frame(codecContext, rgbData ? frame : nullptr) < 0) {
        std::cerr << "无法发送帧到编码器" << std::endl;
        return false;
    }

    // 接收编码器输出的包
    while (avcodec_receive_packet(codecContext, packet) == 0) {
        // 设置包的流索引
        packet->stream_index = videoStream->index;
        // 缩放包的时间戳到流的时间基
        av_packet_rescale_ts(packet, codecContext->time_base, videoStream->time_base);

        // std::cerr << "Writing packet with PTS " << packet->pts << std::endl;

        // 使用交叉写入包
        if (av_interleaved_write_frame(formatContext, packet) < 0) {
            std::cerr << "无法写入包到格式上下文" << std::endl;
            return false;
        }

        av_packet_unref(packet);
    }

    return true;
}

bool FFmpegWriter::pushFrame(uint8_t* rgbData, int width, int height) {
    FrameData frameData;
    frameData.width = width;
    frameData.height = height;
    frameData.data.assign(rgbData, rgbData + (width * height * 3)); // 拷贝RGB数据

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (frameQueue.size() >= 1000) { // 限制队列大小，避免内存占用过高
            return false;
        }
        frameQueue.push(std::move(frameData)); // 使用std::move提高效率
    }
    queueCV.notify_one();
    return true;
}

void FFmpegWriter::startEncoding() {
    if (isEncoding) return;
    isEncoding = true;
    encodingThread = std::thread(&FFmpegWriter::encodingLoop, this);
}

void FFmpegWriter::stopEncoding() {
    if (!isEncoding) return;
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        isEncoding = false;
    }
    queueCV.notify_all();
    if (encodingThread.joinable())
        encodingThread.join();
}

void FFmpegWriter::encodingLoop() {
    while (isEncoding || !frameQueue.empty()) {
        std::unique_lock<std::mutex> lock(queueMutex);
        queueCV.wait(lock, [this]() { return !frameQueue.empty() || !isEncoding; });

        while (!frameQueue.empty()) {
            FrameData frameData = std::move(frameQueue.front());
            frameQueue.pop();
            lock.unlock();

            if (!encodeFrame(frameData.data.data())) { // 传递拷贝后的数据
                std::cerr << "编码帧失败" << std::endl;
            }

            lock.lock();
        }
    }

    // 刷新编码器
    encodeFrame(nullptr);
}

void FFmpegWriter::finalize() {
    stopEncoding(); // 确保编码线程已停止并处理完所有帧

    // 写入尾部
    if (av_write_trailer(formatContext) < 0) {
        std::cerr << "无法写入文件尾部" << std::endl;
    }

    // 关闭输出文件
    if (!(formatContext->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&formatContext->pb);
    }

    // 释放资源
    if (frame) {
        av_frame_free(&frame);
    }
    if (packet) {
        av_packet_free(&packet);
    }
    if (codecContext) {
        avcodec_free_context(&codecContext);
    }
    if (formatContext) {
        avformat_free_context(formatContext);
    }
    if (swsContext) {
        sws_freeContext(swsContext);
    }
}
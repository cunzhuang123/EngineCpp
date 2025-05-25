// VideoResource.cpp
#include "VideoResourceGpu.h"

// FFmpegInstance 实现
VideoResource::FFmpegInstance::FFmpegInstance()
    : fmt_ctx_(nullptr), codec_ctx_(nullptr), avstream_(nullptr), opts_(nullptr) {}

VideoResource::FFmpegInstance::~FFmpegInstance() {
    close();
}

bool VideoResource::FFmpegInstance::open(const std::string& filename, int stream_index) {
    int ret = avformat_open_input(&fmt_ctx_, filename.c_str(), nullptr, nullptr);
    if (ret < 0) {
        char errBuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errBuf, sizeof(errBuf));
        std::cerr << "无法打开文件：" << filename << " 错误码：" << ret << " 描述：" << errBuf << std::endl;
        return false;
    }

    ret = avformat_find_stream_info(fmt_ctx_, nullptr);
    if (ret < 0) {
        char errBuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errBuf, sizeof(errBuf));
        std::cerr << "无法获取流信息：" << filename << " 错误码：" << ret << " 描述：" << errBuf << std::endl;
        avformat_close_input(&fmt_ctx_);
        fmt_ctx_ = nullptr;
        return false;
    }

    if (stream_index < 0 || stream_index >= fmt_ctx_->nb_streams) {
        std::cerr << "无效的流索引：" << stream_index << std::endl;
        avformat_close_input(&fmt_ctx_);
        fmt_ctx_ = nullptr;
        return false;
    }

    avstream_ = fmt_ctx_->streams[stream_index];
    const AVCodec* codec = avcodec_find_decoder(avstream_->codecpar->codec_id);
    if (!codec) {
        std::cerr << "找不到解码器，codec_id：" << avstream_->codecpar->codec_id << std::endl;
        avformat_close_input(&fmt_ctx_);
        fmt_ctx_ = nullptr;
        return false;
    }

    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_) {
        std::cerr << "无法分配解码器上下文。" << std::endl;
        avformat_close_input(&fmt_ctx_);
        fmt_ctx_ = nullptr;
        return false;
    }

    ret = avcodec_parameters_to_context(codec_ctx_, avstream_->codecpar);
    if (ret < 0) {
        char errBuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errBuf, sizeof(errBuf));
        std::cerr << "无法复制编解码器参数，错误码：" << ret << " 描述：" << errBuf << std::endl;
        avcodec_free_context(&codec_ctx_);
        codec_ctx_ = nullptr;
        avformat_close_input(&fmt_ctx_);
        fmt_ctx_ = nullptr;
        return false;
    }

    ret = av_dict_set(&opts_, "threads", "auto", 0);
    if (ret < 0) {
        std::cerr << "无法设置线程选项，错误码：" << ret << std::endl;
    }

    ret = avcodec_open2(codec_ctx_, codec, &opts_);
    if (ret < 0) {
        char errBuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errBuf, sizeof(errBuf));
        std::cerr << "无法打开解码器，错误码：" << ret << " 描述：" << errBuf << std::endl;
        avcodec_free_context(&codec_ctx_);
        codec_ctx_ = nullptr;
        avformat_close_input(&fmt_ctx_);
        fmt_ctx_ = nullptr;
        return false;
    }

    return true;
}

void VideoResource::FFmpegInstance::close() {
    if (opts_) {
        av_dict_free(&opts_);
        opts_ = nullptr;
    }
    if (codec_ctx_) {
        avcodec_free_context(&codec_ctx_);
        codec_ctx_ = nullptr;
    }
    if (fmt_ctx_) {
        avformat_close_input(&fmt_ctx_);
        fmt_ctx_ = nullptr;
    }
}

int VideoResource::FFmpegInstance::getFrame(AVFrame* frame) {
    int ret = avcodec_receive_frame(codec_ctx_, frame);
    if (ret == AVERROR(EAGAIN)) {
        AVPacket pkt;
        ret = av_read_frame(fmt_ctx_, &pkt);
        if (ret < 0) {
            return ret;
        }

        if (pkt.stream_index != avstream_->index) {
            av_packet_unref(&pkt);
            return AVERROR(EAGAIN); // Continue seeking
        }

        ret = avcodec_send_packet(codec_ctx_, &pkt);
        av_packet_unref(&pkt);
        if (ret < 0) {
            char errBuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errBuf, sizeof(errBuf));
            std::cerr << "avcodec_send_packet 失败，错误码：" << ret << " 描述：" << errBuf << std::endl;
            return ret;
        }

        ret = avcodec_receive_frame(codec_ctx_, frame);
    }

    if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
        char errBuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errBuf, sizeof(errBuf));
        std::cerr << "avcodec_receive_frame 失败，错误码：" << ret << " 描述：" << errBuf << std::endl;
    }

    return ret;
}

void VideoResource::FFmpegInstance::seek(int64_t timestamp) {
    // 使用关键帧寻址
    int ret = av_seek_frame(fmt_ctx_, avstream_->index, timestamp, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        char errBuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errBuf, sizeof(errBuf));
        std::cerr << "av_seek_frame 失败，错误码：" << ret << " 描述：" << errBuf << std::endl;
    }

    // 刷新解码器缓冲区以避免旧帧影响
    avcodec_flush_buffers(codec_ctx_);
}

// VideoResource 类实现
VideoResource::VideoResource(const std::string& filePath)
    : filePath(filePath),
      videoStreamIndex(-1),
      duration(0.0),
      avFrame(nullptr),
      avPacket(nullptr),
      swsContext(nullptr),
      rgbBuffer(nullptr),
      rgbBufferSize(0),
      stopDecoder_(false),
      decoderReady_(false),
      seekRequested_(false),
      seekTimestamp_(0) {}

VideoResource::~VideoResource() {
    destroy();
}

bool VideoResource::initialize() {
    // std::unique_lock<std::mutex> lock(resourceMutex_);

    // // 打开解码器实例
    // if (!decoder_.open(filePath, videoStreamIndex)) {
    //     std::cerr << "无法初始化解码器。" << std::endl;
    //     return false;
    // }

    // // 获取视频持续时间
    // AVStream* stream = const_cast<AVStream*>(decoder_.getStream());
    // if (stream->duration != AV_NOPTS_VALUE) {
    //     duration = stream->duration * av_q2d(stream->time_base);
    // } else {
    //     duration = 0.0;
    // }

    // // 初始化解码相关
    // avFrame = av_frame_alloc();
    // avPacket = av_packet_alloc();
    // if (!avFrame || !avPacket) {
    //     std::cerr << "无法分配 AVFrame 或 AVPacket。" << std::endl;
    //     return false;
    // }

    // width = decoder_.getCodecContext()->width;
    // height = decoder_.getCodecContext()->height;

    // // 初始化 swsContext
    // swsContext = sws_getContext(
    //     width,
    //     height,
    //     decoder_.getCodecContext()->pix_fmt,
    //     width,
    //     height,
    //     AV_PIX_FMT_RGB24,
    //     SWS_BILINEAR,
    //     nullptr,
    //     nullptr,
    //     nullptr
    // );
    // if (!swsContext) {
    //     std::cerr << "无法初始化 swsContext。" << std::endl;
    //     return false;
    // }

    // // 分配 RGB 数据缓冲区
    // rgbBufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
    // rgbBuffer = static_cast<uint8_t*>(av_malloc(rgbBufferSize * sizeof(uint8_t)));
    // if (!rgbBuffer) {
    //     std::cerr << "无法分配 RGB 缓冲区。" << std::endl;
    //     return false;
    // }

    // // 生成顶点数据
    // generateVertices();

    // // 启动解码线程
    // stopDecoder_ = false;
    // decoderThread_ = std::thread(&VideoResource::decodingLoop, this);

    // // 等待解码线程准备好
    // cacheCondVar_.wait(lock, [this]() { return decoderReady_.load(); });

    // return true;





        std::unique_lock<std::mutex> lock(resourceMutex_);

    // 打开解码器实例
    // 查找第一个视频流
    AVFormatContext* fmt_ctx = nullptr;
    if (avformat_open_input(&fmt_ctx, filePath.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "无法打开输入文件：" << filePath << std::endl;
        return false;
    }

    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        std::cerr << "无法获取流信息：" << filePath << std::endl;
        avformat_close_input(&fmt_ctx);
        return false;
    }

    // 查找视频流
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; ++i) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = static_cast<int>(i);
            break;
        }
    }

    if (videoStreamIndex == -1) {
        std::cerr << "找不到视频流：" << filePath << std::endl;
        avformat_close_input(&fmt_ctx);
        return false;
    }

    avformat_close_input(&fmt_ctx);

    // 打开解码器
    if (!decoder_.open(filePath, videoStreamIndex)) {
        std::cerr << "无法初始化解码器。" << std::endl;
        return false;
    }

    // 获取视频持续时间
    AVStream* stream = const_cast<AVStream*>(decoder_.getStream());
    if (stream->duration != AV_NOPTS_VALUE) {
        duration = stream->duration * av_q2d(stream->time_base);
    } else {
        // 如果未设置持续时间，可以基于格式估计
        duration = 0.0;
    }

    // 初始化解码相关
    avFrame = av_frame_alloc();
    avPacket = av_packet_alloc();
    if (!avFrame || !avPacket) {
        std::cerr << "无法分配 AVFrame 或 AVPacket。" << std::endl;
        return false;
    }

    width = decoder_.getCodecContext()->width;
    height = decoder_.getCodecContext()->height;

    // 初始化 swsContext
    swsContext = sws_getContext(
        width,
        height,
        decoder_.getCodecContext()->pix_fmt,
        width,
        height,
        AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        nullptr,
        nullptr,
        nullptr
    );
    if (!swsContext) {
        std::cerr << "无法初始化 swsContext。" << std::endl;
        return false;
    }

    // 分配 RGB 数据缓冲区
    rgbBufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
    rgbBuffer = static_cast<uint8_t*>(av_malloc(rgbBufferSize * sizeof(uint8_t)));
    if (!rgbBuffer) {
        std::cerr << "无法分配 RGB 缓冲区。" << std::endl;
        return false;
    }

    // 生成顶点数据
    generateVertices();

    // 启动解码线程
    stopDecoder_ = false;
    decoderThread_ = std::thread(&VideoResource::decodingLoop, this);

    // 等待解码线程准备好
    while (!decoderReady_.load()) {
        cacheCondVar_.wait(lock);
    }

    return true;
}

void VideoResource::generateVertices() {
    float w = static_cast<float>(width) / 2.0f;
    float h = static_cast<float>(height) / 2.0f;

    // x, y, z, u, v
    vertices = {
        -w,  h, 0.0f, 0.0f, 1.0f,
         w,  h, 0.0f, 1.0f, 1.0f,
        -w, -h, 0.0f, 0.0f, 0.0f,
         w, -h, 0.0f, 1.0f, 0.0f
    };
}




bool VideoResource::getFrameAt(double time, std::function<void(uint8_t* data, int width, int height)> callback) {
    using namespace std::chrono;
    auto overall_start = high_resolution_clock::now();

    std::unique_lock<std::mutex> lock(resourceMutex_);

    if (!decoder_.getFormatContext() || !decoder_.getCodecContext()) {
        std::cerr << "错误：decoder 未初始化。" << std::endl;
        return false;
    }

    if (time < 0 || (duration > 0.0 && time > duration)) {
        std::cerr << "错误：请求的时间点超出视频时长。" << std::endl;
        return false;
    }

    // 转换时间到流的时间基
    int64_t timestamp = av_rescale_q(static_cast<int64_t>(time * AV_TIME_BASE),
                                     AVRational{1, AV_TIME_BASE},
                                     decoder_.getStream()->time_base);

    std::cerr << "目标时间（秒）： " << time << " 转换后的 timestamp： " << timestamp << std::endl;

    // 设置 seek 请求
    seekRequested_ = true;
    seekTimestamp_ = timestamp;

    // 通知解码线程执行 seek
    cacheCondVar_.notify_one();

    // 等待解码线程完成 seek
    cacheCondVar_.wait(lock, [this]() { return !seekRequested_; });

    // 等待直到缓存中有帧可用
    while (frameCache_.empty() && !stopDecoder_) {
        cacheCondVar_.wait(lock);
    }

    if (stopDecoder_) {
        std::cerr << "错误：解码线程已停止。" << std::endl;
        return false;
    }

    // 查找缓存中最接近目标时间的帧
    AVFrame* targetFrame = nullptr;
    double minDiff = std::numeric_limits<double>::max();
    double closestTime = 0.0;

    for (auto& frame : frameCache_) {
        double frameTime = frame->best_effort_timestamp * av_q2d(decoder_.getStream()->time_base);
        double diff = std::abs(frameTime - time);
        if (diff < minDiff && frameTime >= time) {
            minDiff = diff;
            closestTime = frameTime;
            targetFrame = frame;
            break; // 找到第一个 >= 目标时间的帧
        }
    }

    if (targetFrame) {
        // 移除缓存中直到目标帧
        while (!frameCache_.empty()) {
            AVFrame* frame = frameCache_.front();
            frameCache_.pop_front();
            if (frame == targetFrame) {
                break;
            }
            av_frame_free(&frame);
        }

        // 解锁以进行 sws_scale
        lock.unlock();

        // 转换帧到 RGB
        uint8_t* dest[4] = { rgbBuffer, nullptr, nullptr, nullptr };
        int destLinesize[4] = { width * 3, 0, 0, 0 };

        int ret_scale = sws_scale(
            swsContext,
            targetFrame->data,
            targetFrame->linesize,
            0,
            height,
            dest,
            destLinesize
        );

        if (ret_scale <= 0) {
            std::cerr << "sws_scale 失败，返回值：" << ret_scale << std::endl;
            av_frame_free(&targetFrame);
            return false;
        }

        // 调用回调函数
        try {
            callback(rgbBuffer, width, height);
        }
        catch (const std::exception& e) {
            std::cerr << "回调函数抛出异常：" << e.what() << std::endl;
            av_frame_free(&targetFrame);
            return false;
        }

        // 释放帧
        av_frame_free(&targetFrame);

        auto overall_end = high_resolution_clock::now();
        std::cerr << "整体帧处理耗时: " << duration_cast<milliseconds>(overall_end - overall_start).count() << " ms" << std::endl;

        return true; // 成功找到并处理目标帧
    }

    std::cerr << "未能找到合适的帧。" << std::endl;
    return false;
}

void VideoResource::clearCache() {
    while (!frameCache_.empty()) {
        AVFrame* frame = frameCache_.front();
        frameCache_.pop_front();
        av_frame_free(&frame);
    }
}

void VideoResource::decodingLoop() {
    {
        std::lock_guard<std::mutex> lk(resourceMutex_);
        decoderReady_ = true;
        cacheCondVar_.notify_one();
    }

    while (!stopDecoder_) {
        // 检查是否有请求的 seek
        {
            std::unique_lock<std::mutex> lk(resourceMutex_);
            if (seekRequested_) {
                std::cerr << "解码线程：收到 Seek 请求，timestamp = " << seekTimestamp_ << std::endl;
                decoder_.seek(seekTimestamp_);
                clearCache();
                seekRequested_ = false;
                cacheCondVar_.notify_one(); // 通知主线程 Seek 已完成
            }
        }

        // 解码下一帧
        int ret = decoder_.getFrame(avFrame);
        if (ret == AVERROR_EOF) {
            std::cerr << "解码线程：到达文件末尾。" << std::endl;
            break;
        }
        else if (ret < 0) {
            std::cerr << "解码线程：解码错误，错误码：" << ret << std::endl;
            break;
        }

        // 克隆帧以存入缓存
        AVFrame* frameCopy = av_frame_clone(avFrame);
        if (!frameCopy) {
            std::cerr << "解码线程：无法克隆 AVFrame。" << std::endl;
            break;
        }

        // 将帧添加到缓存中
        {
            std::lock_guard<std::mutex> lk(resourceMutex_);
            if (frameCache_.size() >= maxCacheSize_) {
                // 缓存已满，移除最旧的帧
                AVFrame* oldFrame = frameCache_.front();
                frameCache_.pop_front();
                av_frame_free(&oldFrame);
            }
            frameCache_.push_back(frameCopy);
            cacheCondVar_.notify_one(); // 通知主线程有新帧可用
        }
    }

    std::cerr << "解码线程：退出。" << std::endl;
}

void VideoResource::destroy() {
    {
        std::lock_guard<std::mutex> lock(resourceMutex_);
        stopDecoder_ = true;
        cacheCondVar_.notify_all();
    }

    if (decoderThread_.joinable()) {
        decoderThread_.join();
    }

    std::lock_guard<std::mutex> lock(resourceMutex_);

    clearCache();

    if (avFrame) {
        av_frame_free(&avFrame);
        avFrame = nullptr;
    }
    if (avPacket) {
        av_packet_free(&avPacket);
        avPacket = nullptr;
    }
    if (decoder_.getCodecContext()) {
        decoder_.close();
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

int VideoResource::getWidth() const {
    return width;
}

int VideoResource::getHeight() const {
    return height;
}

double VideoResource::getDuration() const {
    return duration;
}

const std::vector<float>& VideoResource::getVertices() const {
    return vertices;
}
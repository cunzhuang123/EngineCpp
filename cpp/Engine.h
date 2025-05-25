// Engine.h

#ifndef ENGINE_H
#define ENGINE_H

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN   // 可选：减少 Windows 头体积
#include <windows.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <future>
#include <chrono>
#include <functional>
#include "nlohmann/json.hpp"

#include <chrono>

#include <algorithm> // 包含 std::max 的定义
#include <iostream>
#include <fstream>           // 用于文件操作

// 包含依赖类的头文件
#include "TrackUtils.h"
#include "CoreUtils.h"
#include "src/ShaderManager.h"
#include "src/RenderPass.h"
#include "src/Camera.h"
#include "src/VideoRenderer.h"
#include "src/TransitionRenderer.h"
#include "src/PluginRenderer.h"
#include "src/VideoResource.h"
#include "src/TextResource.h"
#include "src/ImageResource.h"
#include "src/RendererResource.h"
#include "src/FFmpegWriter.h"
#include "src/Materials.h"

class Engine {
public:
    Engine();
    ~Engine();

    bool Init(int width, int height, bool isVisible);
    void UpdateTracks(const nlohmann::json& tracksJson);
    void Play(double startTime, double endTime, double stepTime, bool isDebug, std::string outputPath, int fps, int mBitRate);

    GLuint getNdcbuffer() { return ndcBuffer; };
    RenderTargetInfo getSequenceRenderTargetInfo() { return sequenceRenderTargetInfo; };
    int getRenderTargetWidth() { return renderTargetWidth; };
    int getRenderTargetHeight() { return renderTargetHeight; };

    std::unique_ptr<RenderPass> renderPass;

private:
    std::shared_ptr<ShaderManager> shaderManager;
    std::map<std::string, std::shared_ptr<VideoRenderer>> rendererMap;
    std::map<std::string, std::shared_ptr<TransitionRenderer>> transitionRendererMap;
    std::map<std::string, std::shared_ptr<PluginRenderer>> pluginRendererMap;
    std::vector<std::vector<nlohmann::json>> sequences; // 直接使用 JSON 对象

    std::shared_ptr<Camera> camera;
    std::shared_ptr<Camera> screenCamera;
    GLuint screenBuffer;
    GLuint ndcBuffer;
    double currentTime;
    std::unique_ptr<TrackUtils> trackUtils;
    std::unique_ptr<CoreUtils> coreUtils;
    int renderTargetWidth;
    int renderTargetHeight;
    RenderTargetInfo sequenceRenderTargetInfo;
    RenderTargetInfo defaultRenderTargetInfo;
    std::shared_ptr<Material> finalBlitMaterial;

    GLFWwindow* window;

    // 离屏渲染资源
    GLuint offscreenFbo = 0;
    GLuint offscreenColorTex = 0;
    GLuint offscreenDepthRb  = 0;
    

    // 辅助方法
    void setBlendingMode(const std::string& mode);
    bool render(FFmpegWriter& writer, const std::vector<std::vector<nlohmann::json>>& sequences, int& index, int& nextIndex, GLuint pboIds[2], bool isDebug);
    void updateCamera();
    void updateRenderer(std::shared_ptr<VideoRenderer> renderer, const nlohmann::json& sequence);
    bool isVideoResource(const std::string& filePath);
};

#endif // ENGINE_H
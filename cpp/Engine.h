// Engine.h

#ifndef ENGINE_H
#define ENGINE_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <future>
#include "nlohmann/json.hpp"

// 包含依赖类的头文件
#include "TrackUtils.h"
#include "CoreUtils.h"
#include "src/ShaderManager.h"
#include "src/RenderPass.h"
#include "src/Camera.h"
#include "src/VideoRenderer.h"
#include "src/TransitionRenderer.h"
#include "src/PluginRenderer.h"
#include "src/FFmpegWriter.h"
#include "src/Materials.h"

class Engine {
public:
    Engine();
    ~Engine();

    bool Init(int width, int height, float globalRenderScale, bool isVisible);
    void UpdateTracks(const nlohmann::json& tracksJson);
    void Play(double startTime, double endTime, double stepTime, bool isDebug, std::string outputPath, int fps, int mBitRate);

    GLuint getNdcBuffer() const { return ndcBuffer; };
    RenderTargetInfo getSequenceRenderTargetInfo() { return sequenceRenderTargetInfo; };
    int getRenderTargetWidth() const { return renderTargetWidth; };
    int getRenderTargetHeight() const { return renderTargetHeight; };

    std::unique_ptr<RenderPass> renderPass;
    float globalRenderScale = 1.0f;

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
    
    // 播放控制
    bool isPlaying;
    bool isPaused;
    
    // 键盘事件处理
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
};

#endif // ENGINE_H
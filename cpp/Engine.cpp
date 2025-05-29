// Engine.cpp

#include "Engine.h"
#include <cmath>
#include <thread>
#include "src/ExpressTool.h"
#include "src/ScopedProfiler.h"
#include "Keyframe.h"


#include "src/VideoResource.h"
#include "src/ImageResource.h"
#include "src/TextResource.h"



// 构造函数
Engine::Engine()
    : shaderManager(nullptr),
      renderPass(nullptr),
      screenBuffer(0),
      ndcBuffer(0),
      currentTime(0.0),
      trackUtils(std::make_unique<TrackUtils>()),
      coreUtils(std::make_unique<CoreUtils>()),
      renderTargetWidth(0),
      renderTargetHeight(0)
{
    camera = std::make_shared<Camera>();
    screenCamera = std::make_shared<Camera>();
}

// 析构函数
Engine::~Engine() {
    // 清理资源
    if (offscreenDepthRb)  glDeleteRenderbuffers(1, &offscreenDepthRb);
    if (offscreenColorTex) glDeleteTextures(1, &offscreenColorTex);
    if (offscreenFbo)      glDeleteFramebuffers(1, &offscreenFbo);

}

// 初始化 Engine
bool Engine::Init(int width, int height, float globalRenderScale, bool isVisible) {
    this->globalRenderScale = globalRenderScale;
        // 初始化 GLFW
    if (!glfwInit()) {
        std::cerr << "无法初始化 GLFW" << std::endl;
        return false;
    }

    if (!isVisible)
    {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }
    GLFWwindow* window = glfwCreateWindow(width, height, "Video Rendering", NULL, NULL);
    if (!window) {
        std::cerr << "无法创建窗口" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);

    // 初始化 GLEW
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "无法初始化 GLEW" << std::endl;
        glfwTerminate();
        return false;
    }

    std::clog << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::clog << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::clog << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::clog << "Vendor: " << glGetString(GL_VENDOR) << std::endl;

    // if (GLEW_VERSION_3_3) {
    //     std::clog << "OpenGL 3.3 is supported." << std::endl;
    // } else {
    //     std::clog << "OpenGL 3.3 is not supported. Application may not work correctly." << std::endl;
    //     // 处理错误
    // }


        
    this->window = window;
    this->renderTargetWidth = width;
    this->renderTargetHeight = height;


    // ---------- 创建 1920×1080 离屏 FBO ----------
    glGenFramebuffers(1, &offscreenFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, offscreenFbo);

    // 颜色纹理
    glGenTextures(1, &offscreenColorTex);
    glBindTexture(GL_TEXTURE_2D, offscreenColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, renderTargetWidth, renderTargetHeight,
                0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                        GL_TEXTURE_2D, offscreenColorTex, 0);

    // 深度/模板（如用不到可省略）
    glGenRenderbuffers(1, &offscreenDepthRb);
    glBindRenderbuffer(GL_RENDERBUFFER, offscreenDepthRb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
                        renderTargetWidth, renderTargetHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, offscreenDepthRb);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "离屏 FBO 创建失败\n";
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    updateCamera();

    glGenBuffers(1, &ndcBuffer);
    float ndcVertices[] =  {
        // x, y, z, u, v
        -1, 1, 0, 0, 1,
        1, 1, 0, 1, 1,
        -1, -1, 0, 0, 0,
        1, -1, 0, 1, 0,
    };
    
    glBindBuffer(GL_ARRAY_BUFFER, ndcBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ndcVertices), ndcVertices, GL_STATIC_DRAW);

    defaultRenderTargetInfo = {"screen", width, height};

    sequenceRenderTargetInfo = {"sequenceRenderTarget", width, height};

    this->shaderManager = std::make_shared<ShaderManager>();
    this->renderPass = std::make_unique<RenderPass>(shaderManager, renderTargetWidth, renderTargetHeight, defaultRenderTargetInfo, offscreenFbo);

    finalBlitMaterial = std::make_shared<Material>(Blit);
    finalBlitMaterial->passName = "finalBlitPass";
    finalBlitMaterial->attributeBuffer = ndcBuffer;
    finalBlitMaterial->uniforms["u_texture"].type = UniformType::RenderTarget;
    finalBlitMaterial->uniforms["u_texture"].value = this->sequenceRenderTargetInfo;//.get(); // 存储 Material* 指针
    finalBlitMaterial->renderTargetInfo = defaultRenderTargetInfo; // 渲染到屏幕



    return true;
}

// 更新画布（例如窗口大小改变）

// 设置混合模式
void Engine::setBlendingMode(const std::string& mode) {
    if (mode == "normal") {
        // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_FUNC_ADD);
    } else if (mode == "additive") {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glBlendEquation(GL_FUNC_ADD);
    } else if (mode == "multiply") {
        glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_FUNC_ADD);
    } else if (mode == "subtract") {
        glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_ONE);
        glBlendEquation(GL_FUNC_SUBTRACT);
    } else {
        std::cerr << "Unknown blending mode: " << mode << ". Defaulting to 'normal'.\n";
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_FUNC_ADD);
    }
}

// 渲染当前帧
bool Engine::render(FFmpegWriter& writer, const std::vector<std::vector<nlohmann::json>>& sequences, int& index, int& nextIndex, GLuint pboIds[2], bool isDebug) {
    ScopedProfiler profiler("Engine::Render");

    // 计算全局时间（毫秒）
    double globalTime = currentTime * 1000.0;

    std::vector<std::shared_ptr<Material>> visibleRendererMaterials;

    // bool nextRenderPassAdded = false;
    // 判断哪些渲染器可见
    for (size_t i = 0; i < sequences.size(); i++)
    {
        std::shared_ptr<TransitionRenderer> currentTransitionRenderer = nullptr;
        std::set<std::shared_ptr<VideoRenderer>> visibleRenderers;    
        const auto& sequenceArray = sequences[i];
        for (size_t j = 0; j < sequenceArray.size(); j++)
        {
            const auto& sequence = sequenceArray[j];
            if (!sequence.is_object()) continue;
            if (!sequence.contains("id")) continue;
            std::string seqId = sequence["id"];

            // 判断是否可见
            bool isVisible = trackUtils->isVisibleAtTime(globalTime, sequence);

            if (sequence["type"].get<std::string>() == "plugin")
            {
                auto rendererIt = pluginRendererMap.find(seqId);
                if (rendererIt == pluginRendererMap.end()) {
                    continue; // 找不到渲染器
                }
                auto renderer = rendererIt->second;
                if (isVisible) {
                    if (renderer->hasUniformTime)
                    {
                        double time = trackUtils->getSequenceTime(globalTime, sequence);
                        renderer->updateTime(static_cast<float>(time/1000));
                    }
                    visibleRendererMaterials.push_back(renderer->getMaterialPass());
                    Keyframe::updatePluginRenderer(*renderer, globalTime, sequence, *this);
                }
            }
            else 
            {
                auto rendererIt = rendererMap.find(seqId);
                if (rendererIt == rendererMap.end()) {
                    continue; // 找不到渲染器
                }
                auto renderer = rendererIt->second;
        
                std::shared_ptr<RendererResource> resource = renderer->getRendererResource();
                std::shared_ptr<VideoResource> videoResource = std::dynamic_pointer_cast<VideoResource>(resource);
                if (videoResource)
                {
                    // 获取 originalTime
                    double originalTime = trackUtils->getOriginalTime(globalTime, sequence);
                    ScopedProfiler profilerVideoResource("sequence videoResource-》" + seqId);
                    videoResource->getFrameAt(fmod(originalTime/1000.f, videoResource->getDuration()));
                }
        
                if (isVisible) {
                    renderer->setRenderTarget(sequenceRenderTargetInfo);
                    visibleRenderers.insert(renderer);
                    visibleRendererMaterials.push_back(renderer->getMaterialPass());
                    Keyframe::updateRenderer(*renderer, globalTime, sequence, *this);
                }
            }

            if (sequence.contains("transition"))
            {
                const auto& transition = sequence["transition"];
                std::string transitionId = transition["id"];
                auto transitionRendererIt = transitionRendererMap.find(transitionId);
                if (transitionRendererIt == transitionRendererMap.end()) {
                    continue; // 找不到渲染器
                }
                auto transitionRenderer = transitionRendererIt->second;

                double transitionTime = trackUtils->getTransitionTime(globalTime, transition, sequence);
                double transitionDuration = transition["duration"];
                if (transitionTime >= 0 && transitionTime < transitionDuration)
                {
                    double time = transitionTime/transitionDuration;
                    transitionRenderer->updateTime(time);
                    currentTransitionRenderer = transitionRenderer;
                }
            }
        }

        if (currentTransitionRenderer)
        {
            RenderTargetInfo renderTargetInfo;
            currentTransitionRenderer->updateRenderTargetInfo(renderTargetInfo);
            auto reuslt = visibleRenderers.find(currentTransitionRenderer->firstRenderer);
            if (reuslt == visibleRenderers.end())
            {
                visibleRendererMaterials.push_back(currentTransitionRenderer->firstRenderer->getMaterialPass());
            }
            reuslt = visibleRenderers.find(currentTransitionRenderer->secondRenderer);
            if (reuslt == visibleRenderers.end())
            {
                visibleRendererMaterials.push_back(currentTransitionRenderer->secondRenderer->getMaterialPass());
            }
            visibleRendererMaterials.push_back(currentTransitionRenderer->getMaterialPass());
        }
    }

    visibleRendererMaterials.push_back(finalBlitMaterial);

    // glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // ① 绑定离屏 FBO
    glBindFramebuffer(GL_FRAMEBUFFER, offscreenFbo);
    glViewport(0, 0, renderTargetWidth, renderTargetHeight);
    // // ② 清屏
    // glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // 清除画布
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 启用深度测试
    // glEnable(GL_DEPTH_TEST);
    // glDepthFunc(GL_LEQUAL);

    glEnable(GL_BLEND);
    setBlendingMode("normal");

    {
        ScopedProfiler profilerVideoResource("renderPass->render");
        //渲染机这里有报错，需要慢慢定位
        renderPass->render(visibleRendererMaterials);
    }

    {
        // ScopedProfiler profilerVideoResource("ffpegWriter->writeFrame");

        // 绑定当前的PBO并启动异步读取
        glBindFramebuffer(GL_FRAMEBUFFER, offscreenFbo);   // ← 加这一行
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pboIds[index]);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, renderTargetWidth, renderTargetHeight, GL_RGB, GL_UNSIGNED_BYTE, 0);

        // 处理上一个PBO中的数据
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pboIds[nextIndex]);
        GLubyte* src = (GLubyte*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
        if (src) {
            // 将像素数据直接传递给FFmpegWriter
            if (!writer.pushFrame(src, renderTargetWidth, renderTargetHeight)) {
                std::cerr << "编码队列已满，跳过该帧" << std::endl;
            }
            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        }

        // 解绑PBO
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        // 交换PBO索引
        index = (index + 1) % 2;
        nextIndex = (nextIndex + 1) % 2;

        if (isDebug)
        {
            // 把 offscreenFbo -> 0
            int winW, winH;
            glfwGetFramebufferSize(window, &winW, &winH);

            glBindFramebuffer(GL_READ_FRAMEBUFFER, offscreenFbo);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);          // 默认帧缓冲
            glBlitFramebuffer(0, 0, renderTargetWidth, renderTargetHeight,
                            0, 0, winW, winH,
                            GL_COLOR_BUFFER_BIT, GL_LINEAR);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    return true;
}

// 更新相机设置
void Engine::updateCamera() {
    glGenBuffers(1, &screenBuffer);

    // 设置正交投影
    float aspect = renderTargetWidth / static_cast<float>(renderTargetHeight);
    float orthoWidth = renderTargetWidth*1.0f;
    float orthoHeight = renderTargetHeight*1.0f;
    float cameraNear = 0.1f;
    float cameraFar = 2000.0f;

    float fovy = 2.0f * atan((orthoHeight / 2.0f) / camera->getPosition().z);
    camera->setPerspective(fovy, aspect, cameraNear, cameraFar);

    // 创建顶点数据
    float w = orthoWidth / 2.0f;
    float h = orthoHeight / 2.0f;
    float vertices[] = {
        -w,  h, 0.0f, 0.0f, 1.0f,
         w,  h, 0.0f, 1.0f, 1.0f,
        -w, -h, 0.0f, 0.0f, 0.0f,
         w, -h, 0.0f, 1.0f, 0.0f,
    };

    glBindBuffer(GL_ARRAY_BUFFER, screenBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    screenCamera->setOrthographic(-orthoWidth / 2.0f, orthoWidth / 2.0f, -orthoHeight / 2.0f, orthoHeight / 2.0f, 0.0f, 2000.0f);
}

// 根据序列调整更新渲染器属性
void Engine::updateRenderer(std::shared_ptr<VideoRenderer> renderer, const nlohmann::json& sequence) {
    // 设置位置
    float posX = sequence["adjust"]["transform"]["x"].get<float>() * static_cast<float>(renderTargetWidth);
    float posY = -sequence["adjust"]["transform"]["y"].get<float>() * static_cast<float>(renderTargetHeight);
    glm::vec3 position = { posX, posY, 0.0f };

    // 设置锚点
    glm::vec3 anchor = {0.0f, 0.0f, 0.0f};

    // 设置旋转（将度转换为弧度）
    float rotationDegrees = sequence["adjust"]["rotate"].get<float>();
    float rotationRadians = rotationDegrees * static_cast<float>(M_PI / 180.0f);
    glm::vec3 rotation = {0.0f, 0.0f, rotationRadians};

    // 应用变换
    renderer->setPosition(position);
    renderer->setAnchor(anchor);
    renderer->setRotation(rotation);

    std::string trackType = sequence.value("type", "");
    // 设置缩放
    if (trackType == "text") {
        glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
        renderer->setScale(scale);
    } else {
        // 假设 TrackUtils 有一个方法计算缩放
        float resourceHeight = renderer->getRendererResource()->getHeight()*1.0f;
        float resourceWidth = renderer->getRendererResource()->getWidth()*1.0f;

        std::array<float, 2> sequenceScale = {
            sequence["adjust"]["scale"]["x"].get<float>(),
            sequence["adjust"]["scale"]["y"].get<float>()
        };
        glm::vec3 scale = trackUtils->getSequenceScale(resourceHeight, static_cast<float>(renderTargetHeight),
                                     resourceWidth, static_cast<float>(renderTargetWidth), sequenceScale);
        renderer->setScale(scale);
    }

    // 设置透明度（alpha 通道）
    float opacity = sequence["adjust"]["opacity"].get<float>();
    auto color = glm::vec4(1.0f, 1.0f, 1.0f, opacity);
    renderer->setColor(color);

    renderer->updateVerticeBuffer();
    // 更新材质统一变量
    renderer->updateMaterialUniforms();
}


// 定义一个函数来判断文件是否为视频资源
bool Engine::isVideoResource(const std::string& filePath) {
    // 定义支持的视频文件扩展名（小写）
    const std::vector<std::string> videoExtensions = {
        ".mp4", ".avi", ".mov", ".mkv", ".flv", ".wmv", ".mpeg", ".mpg", ".m4v", ".3gp", ".webm"
    };
    
    // 找到文件路径中最后一个 '.' 的位置
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos == std::string::npos) {
        // 没有找到扩展名
        return false;
    }
    
    // 提取扩展名并转换为小写
    std::string ext = filePath.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    // 遍历视频扩展名列表，检查是否匹配
    for (const auto& videoExt : videoExtensions) {
        if (ext == videoExt) {
            return true;
        }
    }
    
    return false;
}

// 从 JSON 数据更新 tracks
void Engine::UpdateTracks(const nlohmann::json& tracksJsons) {
    // std::map<std::string, std::shared_ptr<VideoRenderer>> newRendererMap;
    // std::vector<nlohmann::json> newSequences;

    std::map<std::string, std::shared_ptr<RendererResource>> rendererResourceMap;

    rendererMap.clear();
    sequences.clear();
    transitionRendererMap.clear();
    pluginRendererMap.clear();
    // 按顺序迭代 tracks
    const nlohmann::json& tracks = tracksJsons["tracks"];
    for (int i = static_cast<int>(tracks.size()) - 1; i >= 0;i--)
    {
        const auto& trackJson = tracks[i];
        if (!trackJson.contains("visible") || !trackJson["visible"].get<bool>()) {
            continue;
        }

        std::string trackId = trackJson.value("id", "");
        std::string trackType = trackJson.value("type", "");

        if (!trackJson.contains("sequences") || !trackJson["sequences"].is_array()) {
            continue;
        }

        std::vector<nlohmann::json> sequenceArray;
        // 迭代 sequences
        for (const auto& sequence : trackJson["sequences"]) {
            if (!sequence.contains("id")) continue;

            std::string seqId = sequence["id"].get<std::string>();

            // 渲染器不存在，创建新的
            std::shared_ptr<RendererResource> resource = nullptr;

            if (trackType == "plugin") {
                auto plugins = sequence["plugins"].get<std::vector<nlohmann::json>>();
                if (plugins.size() > 0)
                {
                    std::shared_ptr<PluginRenderer> renderer = std::make_shared<PluginRenderer>(*this, seqId);
                    // 添加到新的渲染器映射表
                    pluginRendererMap[seqId] = renderer;
                    // 添加到新的序列列表
                    sequenceArray.push_back(sequence);    
                }
            }
            else 
            {
                std::string resourcePath = sequence["resource"].value("absolutePath", "");
                if (trackType == "graphic") {
                    if (isVideoResource(resourcePath))
                    {
                        resource = std::make_shared<VideoResource>(resourcePath);
                    }
                    else 
                    {
                        resource = std::make_shared<ImageResource>(resourcePath);
                    }
                } else if (trackType == "text") {
                    std::optional<std::array<double, 4>> color = CoreUtils::convertHexToColorArray(sequence["resource"].value("color", "#db1116ff"));
                    std::optional<std::array<double, 4>> strokeColor = CoreUtils::convertHexToColorArray(sequence["resource"].value("strokeColor", "#db1116ff"));
                    bool isStroke = sequence["resource"].value("strokeEnabled", false);
                    int strokeWidth = isStroke ? static_cast<int>(sequence["resource"].value("strokeWidth", 0)*globalRenderScale) : 0;
                    int fontSize = static_cast<int>(sequence["resource"].value("fontSize", 60)*sequence["adjust"]["scale"]["x"].get<float>()*globalRenderScale);
                    resource = std::make_shared<TextResource>(resourcePath, sequence["resource"].value("text", "xxx"), fontSize, color, strokeWidth, strokeColor);
                }

                if (!resource) {
                    std::cerr << "Failed to load resource for sequence ID: " << seqId << "\n";
                    continue; // 跳过这个序列
                }

                std::shared_ptr<VideoRenderer> renderer = std::make_shared<VideoRenderer>(camera, screenCamera, screenBuffer, seqId);
                renderer->setRendererResource(resource);
                bool initialized = renderer->initialize(sequence["resource"].value("rotate", 0), sequenceRenderTargetInfo);

                if (initialized && renderer) {
                    // 添加到新的渲染器映射表
                    rendererMap[seqId] = renderer;
                    // 添加到新的序列列表
                    sequenceArray.push_back(sequence);

                    std::string resourceId = sequence["resource"].value("id", "");
                    resourceId = seqId + "_" + resourceId;
                    if (!resourceId.empty())
                    {
                        rendererResourceMap["bufferResourceId:" + resourceId] = resource;
                        rendererResourceMap["textureResourceId:" + resourceId] = resource;
                    }
                }
            }
        }
        sequences.push_back(sequenceArray);

        for (size_t j = 0; j < trackJson["sequences"].size(); j++)
        {
            const auto& sequence = trackJson["sequences"][j];
            if (sequence.contains("transition") && (j + 1) < trackJson["sequences"].size())
            {

                const auto& nextSequence = trackJson["sequences"][j + 1];
                const auto& transition = sequence["transition"];
                std::string sequenceId = sequence["id"];
                std::string nextSequenceId = nextSequence["id"];
                std::string transitionId = transition["id"];

                auto firstRenderer = rendererMap[sequenceId];
                auto secondRenderer = rendererMap[nextSequenceId];
                // std::string id = sequenceId + nextSequenceId + transitionId; 
                auto transitionRenderer = std::make_shared<TransitionRenderer>(firstRenderer, secondRenderer, transitionId, *this);
                transitionRendererMap[transitionId] = transitionRenderer;
            }
        }
    }


    // // 更新渲染器映射和序列列表
    // rendererMap = newRendererMap;
    // sequences = newSequences;

    const nlohmann::json& materialData = tracksJsons["materialData"];
    for (size_t i = 0; i < sequences.size(); i++)
    {
        const auto& sequenceArray = sequences[i];
        for (size_t j = 0; j < sequenceArray.size(); j++)
        {
            const auto& sequence = sequenceArray[j];
            if (!sequence.is_object()) continue;
            if (!sequence.contains("id")) continue;
            std::string seqId = sequence["id"];

            if (sequence["type"].get<std::string>() == "plugin")
            {
                auto rendererIt = pluginRendererMap.find(seqId);
                if (rendererIt == pluginRendererMap.end()) {
                    continue; // 找不到渲染器
                }
                auto renderer = rendererIt->second;

                if (sequence["plugins"].is_array())
                {
                    auto plugins = sequence["plugins"].get<std::vector<nlohmann::json>>();
                    renderer->updatePlugin(sequence, plugins, materialData, rendererResourceMap, screenBuffer, ndcBuffer);
                }
            }
            else{
                auto rendererIt = rendererMap.find(seqId);
                if (rendererIt == rendererMap.end()) {
                    continue; // 找不到渲染器
                }
                auto renderer = rendererIt->second;

                if (sequence["plugins"].is_array())
                {
                    auto plugins = sequence["plugins"].get<std::vector<nlohmann::json>>();
                    if (plugins.size() > 0)
                    {
                        auto material = Material::deserialize(materialData, rendererResourceMap, screenBuffer, ndcBuffer, seqId, sequenceRenderTargetInfo);
                        if(material)
                        {
                            renderer->setMaterialTexturePass(material);
                        }
                        for (int j = 0; j < plugins.size(); j++)
                        {
                            auto& plugin = plugins[j];
                            const auto& expressValue = ExpressTool::collectMaterialExpressValue(renderer->getRendererResource(), renderer->getName(), renderer->getMaterialPass(), plugin, j, sequenceRenderTargetInfo);
                            ExpressTool::caculateMaterialExpress(renderer->getName(), renderer->getMaterialPass(), expressValue, j);
                        }
                    }
                }
            }

            if (sequence.contains("transition"))
            {
                const auto& transition = sequence["transition"].get<nlohmann::json>();
                if (transition.contains("id"))
                {
                    const auto& transition = sequence["transition"];
                    std::string sequenceId = sequence["id"];
                    std::string transitionId = transition["id"];
                    auto transitionRenderer = transitionRendererMap[transitionId];
                    transitionRenderer->setMaterialPass(Material::deserialize(materialData, rendererResourceMap, screenBuffer, ndcBuffer, transitionId, sequenceRenderTargetInfo));
                }
            }
        }
    }
    shaderManager->setExtendShader(materialData["shaders"]);


    for (size_t i = 0; i < sequences.size(); i++)
    {
        const auto& sequenceArray = sequences[i];
        for (size_t j = 0; j < sequenceArray.size(); j++)
        {
            const auto& sequence = sequenceArray[j];
            std::string sequenceId = sequence["id"];
            auto rendererIt = rendererMap.find(sequenceId);
            if (rendererIt == rendererMap.end()) {
                continue; // 找不到渲染器
            }
            auto renderer = rendererIt->second;
            updateRenderer(renderer, sequence);
        }
    }
}

// 播放序列
void Engine::Play(double startTime, double endTime, double stepTime, bool isDebug, std::string outputPath, int fps, int mBitRate) {
    FFmpegWriter writer(outputPath, renderTargetWidth, renderTargetHeight, fps, mBitRate); // 仅构造函数，不再调用 initialize
    if (!writer.initialize(outputPath)) { // 初始化必须调用
        std::cerr << "初始化 FFmpeg Writer 失败" << std::endl;
        return;
    }
    writer.startEncoding();

    // 在初始化时创建两个PBO（双缓冲）
    GLuint pboIds[2];
    glGenBuffers(2, pboIds);
    for (int i = 0; i < 2; ++i) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pboIds[i]);
        glBufferData(GL_PIXEL_PACK_BUFFER, renderTargetWidth * renderTargetHeight * 3, nullptr, GL_STREAM_READ);
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    int index = 0;
    int nextIndex = 1;


    currentTime = startTime;
    {
        ScopedProfiler profiler("Engine::Play Render");
        // 播放循环
        while (currentTime < endTime) {
            currentTime += stepTime;
            render(writer, sequences, index,  nextIndex, pboIds, isDebug);
        }
    }

    {
        ScopedProfiler profiler("Engine::Play writer.finalize");

        // 停止编码线程并完成编码
        writer.stopEncoding();
        // Finalize FFmpeg Writer
        writer.finalize();
    }

    glfwTerminate();
}
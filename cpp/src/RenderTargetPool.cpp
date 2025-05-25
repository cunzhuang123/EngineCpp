// RenderTargetPool.cpp

#include "RenderTargetPool.h"
#include <iostream>
#include "ScopedProfiler.h"



RenderTargetPool& RenderTargetPool::instance()
{
    static RenderTargetPool s;
    return s;
}

void RenderTargetPool::initialize(
    GLuint width,
    GLuint height,
    RenderTargetInfo defaultRenderTargetInfo,
    GLuint defaultFramebuffer
)
{
    this->width = width;
    this->height = height;
    this->defaultRenderTargetInfo = defaultRenderTargetInfo;
    this->defaultFramebuffer = defaultFramebuffer;
    defaultRenderTargetInfoName = defaultRenderTargetInfo.name + "_" + std::to_string(defaultRenderTargetInfo.width) + "x" + std::to_string(defaultRenderTargetInfo.height);
}


RenderTargetPool::~RenderTargetPool() {
    reset();
}

std::shared_ptr<RenderTarget> RenderTargetPool::acquire(const RenderTargetInfo& renderTargetInfo, bool hasDepthStencil) {
    int widthTarget = renderTargetInfo.width;
    int heightTarget = renderTargetInfo.height;
    std::string renderTargetName = renderTargetInfo.name + "_" + std::to_string(widthTarget) + "x" + std::to_string(heightTarget);

    // std::cerr << "RenderTargetPool::acquire renderTargetName: " << renderTargetName << std::endl;

    auto it = inUse.find(renderTargetName);
    if (it != inUse.end()) {
        return it->second;
    }

    auto poolIt = pool.find(renderTargetName);
    if (poolIt != pool.end()) {
        glBindFramebuffer(GL_FRAMEBUFFER, poolIt->second->framebuffer);
        // 清除
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        inUse[renderTargetName] = poolIt->second;
        return poolIt->second;
    }

    auto rt = std::make_shared<RenderTarget>();
    if (renderTargetName == defaultRenderTargetInfoName)
    {
        rt->framebuffer = defaultFramebuffer;
        rt->texture = -1;
        rt->depthStencilRBO = -1;
    }
    else 
    {
        if (!generateRenderTarget(rt, widthTarget, heightTarget, renderTargetName, hasDepthStencil)) {
            // std::cerr << "创建渲染目标失败: " << renderTargetName << std::endl;
            return nullptr;
        }
    }

    inUse[renderTargetName] = rt;
    return rt;
}

std::uint64_t RenderTargetPool::makeRTKey(int width, int height, bool depth)
{
    //  |  width(32) | height(31) | depth(1) |
    return  (static_cast<std::uint64_t>(width)  << 32) |
            (static_cast<std::uint64_t>(height) << 1 ) |
            static_cast<std::uint64_t>(depth);
}

std::shared_ptr<RenderTarget> RenderTargetPool::getCachedRenderTarget(std::uint64_t key)
{
    auto it = renderTargetPool.find(key);
    if (it != renderTargetPool.end()) {
        return it->second;
    }
    return nullptr; 
}

void RenderTargetPool::cachedRenderTarget(std::uint64_t key, std::shared_ptr<RenderTarget> rt)
{
    auto it = renderTargetPool.find(key);
    if (it == renderTargetPool.end()) {
         renderTargetPool[key] = rt;
    }
    else {
        // std::cerr << "RenderTargetPool::cachedRenderTarget key already exists" << std::endl;
    }
}

bool RenderTargetPool::generateRenderTarget(std::shared_ptr<RenderTarget> rt, int widthTarget, int heightTarget, std::string renderTargetName, bool hasDepthStencil) {
    {
        // ScopedProfiler profiler("RenderTargetPool::generateRenderTarget glGenFramebuffers");
        glGenFramebuffers(1, &rt->framebuffer);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, rt->framebuffer);

    {
        // ScopedProfiler profiler("RenderTargetPool::generateRenderTarget glGenTextures");
        glGenTextures(1, &rt->texture);
    }

    // std::cerr << "glGenTexture id: " << rt->texture << std::endl;

    glBindTexture(GL_TEXTURE_2D, rt->texture);
    if (widthTarget == 0 || heightTarget == 0)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    }
    else 
    {
        {
            // ScopedProfiler profiler("RenderTargetPool::generateRenderTarget glTexImage2D");
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, widthTarget, heightTarget, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        }
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    {
        // ScopedProfiler profiler("RenderTargetPool::generateRenderTarget glFramebufferTexture2D");
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->texture, 0);
    }

    if (hasDepthStencil)
    {
        // ScopedProfiler profiler("RenderTargetPool::generateRenderTarget DepthStencil");

        // 创建渲染缓冲对象用于深度和模板
        glGenRenderbuffers(1, &rt->depthStencilRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, rt->depthStencilRBO);
        if (widthTarget == 0 || heightTarget == 0) {
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        } else {
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, widthTarget, heightTarget);
        }
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rt->depthStencilRBO);
    }
    else 
    {
        rt->depthStencilRBO = std::numeric_limits<GLuint>::max();
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer 不完整，无法创建用于 renderTargetName: " << renderTargetName << std::endl;
        glDeleteFramebuffers(1, &rt->framebuffer);
        glDeleteTextures(1, &rt->texture);

        if (hasDepthStencil)
        {
            glDeleteRenderbuffers(1, &rt->depthStencilRBO);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}

std::shared_ptr<RenderTarget> RenderTargetPool::getInUseRenderTarget(const RenderTargetInfo& renderTargetInfo)
{
    int widthTarget = renderTargetInfo.width;
    int heightTarget = renderTargetInfo.height;
    std::string renderTargetName = renderTargetInfo.name + "_" + std::to_string(widthTarget) + "x" + std::to_string(heightTarget);

    auto it = inUse.find(renderTargetName);
    if (it != inUse.end()) {
        return it->second;
    }
    return nullptr;
}

void RenderTargetPool::release(const std::string& renderTargetName) {
    auto it = inUse.find(renderTargetName);
    if (it != inUse.end()) {
        pool[renderTargetName] = it->second;
        inUse.erase(it);
    }
}

void RenderTargetPool::releaseUnused() {
    for (auto it = inUse.begin(); it != inUse.end();) {
        release(it->first);
        it = inUse.begin();
    }
}

void RenderTargetPool::reset() {
    auto max = std::numeric_limits<GLuint>::max();

    // 清空池
    for (auto& pair : pool) {
        glDeleteFramebuffers(1, &pair.second->framebuffer);
        glDeleteTextures(1, &pair.second->texture);
        if (pair.second->depthStencilRBO != max) {
            glDeleteRenderbuffers(1, &pair.second->depthStencilRBO);
        }
    }
    pool.clear();

    // 清空正在使用的渲染目标
    for (auto& pair : inUse) {
        glDeleteFramebuffers(1, &pair.second->framebuffer);
        glDeleteTextures(1, &pair.second->texture);
        if (pair.second->depthStencilRBO != max) {
            glDeleteRenderbuffers(1, &pair.second->depthStencilRBO);
        }
    }
    inUse.clear();
}

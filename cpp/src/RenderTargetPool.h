// RenderTargetPool.h

#ifndef RENDERTARGETPOOL_H
#define RENDERTARGETPOOL_H

#include <glad/glad.h>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include "Materials.h"
#include <iostream>
// #include <unordered_set>

using namespace std;

struct RenderTarget {
    GLuint framebuffer;
    GLuint texture;
    GLuint depthStencilRBO;
    // ① 相等判定：三个句柄都一样才算同一个 RenderTarget
    bool operator==(const RenderTarget& rhs) const noexcept
    {
        return framebuffer     == rhs.framebuffer
            && texture         == rhs.texture
            && depthStencilRBO == rhs.depthStencilRBO;
    }
};

// ② 为 RenderTarget 专门化 std::hash
namespace std
{
template<>
struct hash<RenderTarget>
{
    size_t operator()(const RenderTarget& rt) const noexcept
    {
        // GLuint 在大多数实现里是 32 位无符号整型
        // 这里随手用三个整数混合出一个 hash；你也可以改成更复杂的
        size_t h = std::hash<GLuint>{}(rt.framebuffer);
        h ^= (static_cast<size_t>(rt.texture)         << 1);
        h ^= (static_cast<size_t>(rt.depthStencilRBO) << 2);
        return h;
    }
};
}


class RenderTargetPool {
public:
    static RenderTargetPool& instance();    // 引用，不再是 shared_ptr

    
    void initialize(
        GLuint width,
        GLuint height,
        RenderTargetInfo defaultRenderTargetInfo,
        GLuint defaultFramebuffer
    );

    // static std::shared_ptr<RenderTargetPool> getInstance();

    // 禁止拷贝和赋值
    // RenderTargetPool(const RenderTargetPool&) = delete;
    RenderTargetPool& operator=(const RenderTargetPool&) = delete;


    std::shared_ptr<RenderTarget> acquire(const RenderTargetInfo& renderTargetInfo, bool hasDepthStencil = false);
    void release(const std::string& renderTargetName);
    void releaseUnused();
    void reset();

    std::shared_ptr<RenderTarget> getInUseRenderTarget(const RenderTargetInfo& renderTargetInfo); // 添加访问器

    bool generateRenderTarget(std::shared_ptr<RenderTarget> rt, int widthTarget, int heightTarget, std::string renderTargetName, bool hasDepthStencil);

    static std::uint64_t makeRTKey(int width, int height, bool depth);

    std::shared_ptr<RenderTarget> getCachedRenderTarget(std::uint64_t key);
    void cachedRenderTarget(std::uint64_t key, std::shared_ptr<RenderTarget> rt);
    std::map<std::uint64_t, std::shared_ptr<RenderTarget>> renderTargetPool;

private:
    // RenderTargetPool(GLuint width, GLuint height, RenderTargetInfo defaultRenderTargetInfo, GLuint defaultFramebuffer);
    RenderTargetPool() = default;
    ~RenderTargetPool();

    // 构造函数和析构函数设为私有

    GLuint width;
    GLuint height;
    RenderTargetInfo defaultRenderTargetInfo;
    std::string defaultRenderTargetInfoName;
    GLuint defaultFramebuffer;
    std::map<std::string, std::shared_ptr<RenderTarget>> pool;
    std::map<std::string, std::shared_ptr<RenderTarget>> inUse;
    // 静态成员变量存储实例
    // static std::shared_ptr<RenderTargetPool> instance;
    // static std::once_flag initFlag;
};

#endif // RENDERTARGETPOOL_H
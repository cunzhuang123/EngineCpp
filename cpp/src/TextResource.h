// TextResource.h

#ifndef TEXT_RESOURCE_H
#define TEXT_RESOURCE_H

#include <string>
#include <cstdint>
#include <stdexcept>
#include <vector>

extern "C" {
    #include "../nanovg/nanovg.h"
}

#include <ft2build.h>
#include FT_FREETYPE_H

#include "RenderTargetPool.h"
#include "RendererResource.h"
// #include <glm/glm.hpp>


enum class PathCommandType {
    SetWinding, // 设置路径环绕方向
    MoveTo,
    LineTo,
    QuadTo,
    ClosePath
};

// 定义路径命令结构
struct PathCommand {
    PathCommandType type;
    float x;
    float y;
    float cx; // 控制点 x（仅 QuadTo）
    float cy; // 控制点 y（仅 QuadTo）

    // 构造函数
    PathCommand(PathCommandType t, float px = 0, float py = 0, float cpx = 0, float cpy = 0) 
        : type(t), x(px), y(py), cx(cpx), cy(cpy) {}
};

// const static std::shared_ptr<RenderTargetPool> renderTargetPoolTexts = std::make_shared<RenderTargetPool>(256, 256);

class TextResource : public RendererResource {
public:

    TextResource(const std::string& fontFilePath, const std::string& text, int fontSize, const std::optional<std::array<double, 4>>& color, int strokeSize, const std::optional<std::array<double, 4>>& strokeColor);
    virtual ~TextResource();

    virtual bool initialize(int rotate);

    virtual GLuint getWidth() const override;
    virtual GLuint getHeight() const override;

    virtual GLuint getSourceWidth() const override;
    virtual GLuint getSourceHeight() const override;

    virtual const std::vector<float>& getVertices() const override;
    virtual GLuint getTexture() override {return textureId;};

    std::shared_ptr<RenderTarget> getRenderTarget() { return renderTarget; }
private:
    std::u32string utf8_to_u32string(const std::string& str);

    bool isContourClockwise(const FT_Outline* outline, int contourIndex);
    std::vector<PathCommand> parseFTOutline(const FT_Outline* outline, float penX, float penY);
    void drawPath(NVGcontext* vg, const std::vector<PathCommand>& commands);

    std::string fontFilePath;
    std::string text;
    int fontSize;
    std::optional<std::array<double, 4>> color;
    int strokeSize;
    std::optional<std::array<double, 4>> strokeColor;

    int width;
    int height;
    GLuint textureId;
    std::vector<float> vertices;
    std::shared_ptr<RenderTarget> renderTarget;

    void generateVertices();

    void cleanup();
};

#endif // TEXT_RESOURCE_H
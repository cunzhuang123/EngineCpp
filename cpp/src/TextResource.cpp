// TextResource.cpp

#include "TextResource.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <limits>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "ScopedProfiler.h"

#define NANOVG_GL3_IMPLEMENTATION 
#include "../nanovg/nanovg_gl.h"


TextResource::TextResource(const std::string& fontFilePath, const std::string& text, int fontSize, const std::optional<std::array<double, 4>>& color, int strokeSize, const std::optional<std::array<double, 4>>& strokeColor)
    : fontFilePath(fontFilePath), text(text), fontSize(fontSize), color(color), width(0), height(0), textureId(0), strokeSize(strokeSize), strokeColor(strokeColor) {
}

TextResource::~TextResource() {
    cleanup();
}

std::u32string TextResource::utf8_to_u32string(const std::string& str) {
    std::u32string result;
    size_t i = 0;
    while (i < str.size()) {
        char32_t codepoint = 0;
        unsigned char c = static_cast<unsigned char>(str[i]);

        if (c <= 0x7F) {
            // 1-byte sequence (ASCII)
            codepoint = c;
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            // 2-byte sequence
            if (i + 1 >= str.size()) {
                throw std::runtime_error("Invalid UTF-8 sequence");
            }
            unsigned char c1 = static_cast<unsigned char>(str[i + 1]);
            codepoint = ((c & 0x1F) << 6) |
                        (c1 & 0x3F);
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            // 3-byte sequence
            if (i + 2 >= str.size()) {
                throw std::runtime_error("Invalid UTF-8 sequence");
            }
            unsigned char c1 = static_cast<unsigned char>(str[i + 1]);
            unsigned char c2 = static_cast<unsigned char>(str[i + 2]);
            codepoint = ((c & 0x0F) << 12) |
                        ((c1 & 0x3F) << 6) |
                        (c2 & 0x3F);
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            // 4-byte sequence
            if (i + 3 >= str.size()) {
                throw std::runtime_error("Invalid UTF-8 sequence");
            }
            unsigned char c1 = static_cast<unsigned char>(str[i + 1]);
            unsigned char c2 = static_cast<unsigned char>(str[i + 2]);
            unsigned char c3 = static_cast<unsigned char>(str[i + 3]);
            codepoint = ((c & 0x07) << 18) |
                        ((c1 & 0x3F) << 12) |
                        ((c2 & 0x3F) << 6) |
                        (c3 & 0x3F);
            i += 4;
        } else {
            throw std::runtime_error("Invalid UTF-8 sequence");
        }

        result.push_back(codepoint);
    }
    return result;
}

bool TextResource::initialize(int rotate) {
    // ScopedProfiler profiler("TextResource::initialize");

// Initialize FreeType
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "Could not initialize FreeType library." << std::endl;
    }

    // Load font face
    FT_Face face;
    if (FT_New_Face(ft, fontFilePath.c_str(), 0, &face)) {
        std::cerr << "Failed to load font: " << fontFilePath << std::endl;
        FT_Done_FreeType(ft);
    }


    // 设置字符大小
    FT_Set_Char_Size(face, 0, static_cast<FT_F26Dot6>(fontSize * 64), 0, 0);

    // 初始化变量
    float penX = 0.0f; // 当前绘制位置的 x 坐标

    // 用浮点数的最大和最小值来初始化 minX 和 maxX
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();

    float maxHeight = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();

    std::u32string text = utf8_to_u32string(this->text);

    // 遍历文本中的每个字符
    for (char32_t c : text) {
        FT_UInt glyphIndex = FT_Get_Char_Index(face, c);

        if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING)) {
            std::cerr << "Failed to load glyph for character: " << c << std::endl;
            continue;
        }

        // 获取当前字形的度量信息
        FT_Glyph_Metrics& metrics = face->glyph->metrics;

        // 计算字形的左边界和右边界
        float glyphLeft = penX + metrics.horiBearingX / 64.0f;
        float glyphRight = glyphLeft + metrics.width / 64.0f;

        // 更新 minX 和 maxX
        minX = std::min(minX, glyphLeft);
        maxX = std::max(maxX, glyphRight);

        // 计算字形的顶部和底部位置
        float glyphTop = metrics.horiBearingY / 64.0f;
        float glyphBottom = glyphTop - metrics.height / 64.0f;

        // 更新 maxHeight 和 minY
        maxHeight = std::max(maxHeight, glyphTop);
        minY = std::min(minY, glyphBottom);

        // 更新绘制位置
        penX += face->glyph->advance.x / 64.0f;
    }

    // 计算文本的实际宽度和高度
    int realWidth =  std::lround(maxX - minX);
    int realHeight = std::lround(maxHeight - minY);

    GLuint targetWidth = realWidth + strokeSize*2;
    GLuint targetHeight = realHeight + strokeSize*2;
    this->width = targetWidth;
    this->height = targetHeight;


    glViewport(0, 0, targetWidth, targetHeight);


    // std::cerr << "renderTargetInfo.name:" << renderTargetInfo.name << std::endl;

    auto rtKey = RenderTargetPool::makeRTKey(targetWidth, targetHeight, true);

    auto renderTargetName = this->text + std::to_string(fontSize);
    renderTarget = RenderTargetPool::instance().getCachedRenderTarget(rtKey);
    if (renderTarget == nullptr) {
        renderTarget = std::make_shared<RenderTarget>();
        {
            // ScopedProfiler profiler("generateRenderTarget ");
            if (!RenderTargetPool::instance().generateRenderTarget(renderTarget, targetWidth, targetHeight, renderTargetName, true)) {
                std::cerr << "无法获取字体渲染目标用于: " << this->text.c_str() << std::endl;
            }
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, renderTarget->framebuffer);


    NVGcontext* vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);// | NVG_DEBUG
    if (vg == nullptr) {
        std::cerr << "无法初始化 NanoVG" << std::endl;
        // 处理错误
    }

    glfwPollEvents();
    // 清除屏幕
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // Begin NanoVG frame
    nvgBeginFrame(vg, targetWidth*1.0f, targetHeight*1.0f, 1.0f);

    // Start position
    penX = 0.0f;
    float penY = maxHeight; // Move down by max ascent

    // Set fill color
    NVGcolor fillColor = nvgRGBAf(static_cast<float>(color->at(0)), static_cast<float>(color->at(1)), static_cast<float>(color->at(2)), static_cast<float>(color->at(3)));
    nvgFillColor(vg, fillColor);


    if (strokeSize > 0)
    {
        // 设置矩形的描边颜色
        NVGcolor nvStrokeColor = nvgRGBAf(static_cast<float>(strokeColor->at(0)), static_cast<float>(strokeColor->at(1)), static_cast<float>(strokeColor->at(2)), static_cast<float>(strokeColor->at(3)));
        nvgStrokeColor(vg, nvStrokeColor); // 天蓝色
        // 设置矩形的描边宽度
        nvgStrokeWidth(vg, strokeSize*2.0f);
    }


    nvgTranslate(vg, strokeSize - minX, strokeSize*1.0f);//texHeight  (fontSize - height)/2

    for (char32_t c : text) {
        FT_UInt glyphIndex = FT_Get_Char_Index(face, c);

        if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING)) {
            std::cerr << "Failed to load glyph for character: " << static_cast<char>(c) << std::endl;
            continue;
        }

        FT_GlyphSlot slot = face->glyph;

        if (slot->format != FT_GLYPH_FORMAT_OUTLINE) {
            std::cerr << "Glyph format is not outline for character: " << static_cast<char>(c) << std::endl;
            continue;
        }

        if (slot->outline.n_contours <= 0)
        {
            std::cerr << "字体包含不支持的字符: " << this->text << std::endl;
        }

        // 解析轮廓为命令数组
        std::vector<PathCommand> commands = parseFTOutline(&slot->outline, penX, penY);

        // 绘制路径
        drawPath(vg, commands);

        // 更新 penX, penY 根据字形的 advance
        penX += slot->advance.x / 64.0f;
        penY += slot->advance.y / 64.0f;
    }

    nvgEndFrame(vg);

    glBindFramebuffer(GL_FRAMEBUFFER, 0); 

    textureId = renderTarget->texture;

    generateVertices();

    
    return true;
}


// 正确计算轮廓方向的函数
bool TextResource::isContourClockwise(const FT_Outline* outline, int contourIndex) {
    int start = (contourIndex == 0) ? 0 : outline->contours[contourIndex - 1] + 1;
    int end = outline->contours[contourIndex];
    double area = 0.0;

    for (int i = start; i <= end; ++i) {
        int next = (i + 1 > end) ? start : i + 1;
        double x1 = outline->points[i].x / 64.0;
        double y1 = outline->points[i].y / 64.0;
        double x2 = outline->points[next].x / 64.0;
        double y2 = outline->points[next].y / 64.0;
        area += (x1 * y2 - x2 * y1);
    }

    return area < 0.0; // 顺时针为 true，逆时针为 false
}




std::vector<PathCommand> TextResource::parseFTOutline(const FT_Outline* outline, float penX, float penY) {
    std::vector<PathCommand> commands;
    int contourStartIndex = 0;

    for (int contourIndex = 0; contourIndex < outline->n_contours; ++contourIndex) {
        int contourEndIndex = outline->contours[contourIndex];

        // 获取第一个点
        int firstPointIndex = contourStartIndex;
        FT_Vector firstPoint = outline->points[firstPointIndex];

        float x0 = penX + firstPoint.x / 64.0f;
        float y0 = penY - firstPoint.y / 64.0f;
        commands.emplace_back(PathCommandType::MoveTo, x0, y0);

        // 遍历轮廓中的点
        for (int pointIndex = contourStartIndex; pointIndex <= contourEndIndex; ++pointIndex) {
            FT_Vector point = outline->points[pointIndex];
            char tag = outline->tags[pointIndex];

            float x = penX + point.x / 64.0f;
            float y = penY - point.y / 64.0f; // Y轴反转

            if (tag & FT_CURVE_TAG_ON) {
                commands.emplace_back(PathCommandType::LineTo, x, y);
            } else { // 控制点
                int nextIndex = (pointIndex + 1 > contourEndIndex) ? contourStartIndex : pointIndex + 1;
                FT_Vector nextPoint = outline->points[nextIndex];
                char nextTag = outline->tags[nextIndex];
                float nextX = penX + nextPoint.x / 64.0f;
                float nextY = penY - nextPoint.y / 64.0f;

                if (nextTag & FT_CURVE_TAG_ON) {
                    // 二次贝塞尔曲线
                    commands.emplace_back(PathCommandType::QuadTo, nextX, nextY, x, y);
                    ++pointIndex; // 跳过下一个点
                } else {
                    // 两个连续的控制点之间的中点作为终点
                    float midX = (x + nextX) / 2.0f;
                    float midY = (y + nextY) / 2.0f;
                    commands.emplace_back(PathCommandType::QuadTo, midX, midY, x, y);
                }
            }
        }

        commands.emplace_back(PathCommandType::ClosePath, 0.0f, 0.0f);

        // 在这里添加 SetWinding 命令
        bool isClockwise = isContourClockwise(outline, contourIndex);
        commands.emplace_back(PathCommandType::SetWinding, isClockwise ? 1.0f : -1.0f, 0.0f);

        contourStartIndex = contourEndIndex + 1;
    }

    return commands;
}


void TextResource::drawPath(NVGcontext* vg, const std::vector<PathCommand>& commands) {
    nvgBeginPath(vg);

    for (const auto& cmd : commands) {
        switch (cmd.type) {
            case PathCommandType::MoveTo:
                nvgMoveTo(vg, cmd.x, cmd.y);
                break;
            case PathCommandType::LineTo:
                nvgLineTo(vg, cmd.x, cmd.y);
                break;
            case PathCommandType::QuadTo:
                nvgQuadTo(vg, cmd.cx, cmd.cy, cmd.x, cmd.y);
                break;
            case PathCommandType::ClosePath:
                nvgClosePath(vg);
                break;
            case PathCommandType::SetWinding:
                if (cmd.x > 0.0f) {
                    nvgPathWinding(vg, NVG_HOLE); // 顺时针方向表示孔洞
                } else {
                    nvgPathWinding(vg, NVG_SOLID); // 逆时针方向表示实心
                }
                break;
        }
    }
    if (strokeSize > 0)
    {
        nvgStroke(vg);
    }
    // 填充路径
    nvgFill(vg);
}



void TextResource::generateVertices() {
    // 定义四个顶点，以纹理的中心为原点
    float w = width / 2.0f;
    float h = height / 2.0f;

    // 顶点数据：x, y, z, u, v
    vertices = {
        -w,  h, 0.0f, 0.0f, 1.0f,  // 左上角
         w,  h, 0.0f, 1.0f, 1.0f,  // 右上角
        -w, -h, 0.0f, 0.0f, 0.0f,  // 左下角
         w, -h, 0.0f, 1.0f, 0.0f,  // 右下角
    };
}

const std::vector<float>& TextResource::getVertices() const {
    return vertices;
}

GLuint TextResource::getWidth() const {
    return width;
}

GLuint TextResource::getHeight() const {
    return height;
}

GLuint TextResource::getSourceWidth() const {
    return width;
}

GLuint TextResource::getSourceHeight() const {
    return height;
}
void TextResource::cleanup() {
    if (textureId != 0) {
        // glDeleteTextures(1, &textureId);
        textureId = 0;
    }
}
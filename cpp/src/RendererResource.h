#ifndef RENDERERRESOURCE_H
#define RENDERERRESOURCE_H

#include <GL/glew.h>
#include <vector>

class RendererResource {
public:
    RendererResource();
    virtual ~RendererResource(); // 声明为虚析构函数

    virtual bool initialize(int rotate) = 0;

    virtual GLuint getWidth() const = 0;
    virtual GLuint getHeight() const = 0;
    virtual GLuint getSourceWidth() const = 0;
    virtual GLuint getSourceHeight() const = 0;

    virtual const std::vector<float>& getVertices() const = 0;
    virtual GLuint getTexture() = 0;
};

#endif // RENDERERRESOURCE_H

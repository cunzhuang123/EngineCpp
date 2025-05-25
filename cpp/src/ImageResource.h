// ImageResource.h

#ifndef IMAGE_RESOURCE_H
#define IMAGE_RESOURCE_H

#include <string>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include <GL/glew.h>
#include "RendererResource.h"


class ImageResource : public RendererResource {
public:

    ImageResource(const std::string& filePath);
    virtual ~ImageResource();

    virtual bool initialize(int rotate);

    virtual GLuint getWidth() const override;
    virtual GLuint getHeight() const override;
    virtual GLuint getSourceWidth() const override;
    virtual GLuint getSourceHeight() const override;

    virtual const std::vector<float>& getVertices() const override;
    virtual GLuint getTexture() override {return textureId;};

private:
    std::string filePath;
    int width;
    int height;
    GLuint textureId;
    std::vector<float> vertices;

    void generateVertices();
    void cleanup();
};

#endif // IMAGE_RESOURCE_H
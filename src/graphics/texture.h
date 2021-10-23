#ifndef SP_GRAPHICS_TEXTURE_H
#define SP_GRAPHICS_TEXTURE_H

#include "nonCopyable.h"
#include "graphics/image.h"

#include <optional>


namespace sp {

class Texture : sp::NonCopyable
{
public:
    static Image loadUASTC(const P<ResourceStream>& stream, std::optional<glm::uvec2> threshold);
    static size_t compressedSize(const Image& image);
    virtual void bind() = 0;
};

class BasicTexture : public Texture
{
public:
    void setRepeated(bool);
    void setSmooth(bool);
    
    void loadFromImage(Image&& image);

    virtual void bind() override;

private:
    bool smooth = false;
    unsigned int handle = 0;
    sp::Image image;
};

}

#endif//SP_GRAPHICS_TEXTURE_H

#ifndef SP_GRAPHICS_TEXTURE_H
#define SP_GRAPHICS_TEXTURE_H

#include "nonCopyable.h"
#include "graphics/image.h"


namespace sp {

class Texture : sp::NonCopyable
{
public:
    virtual void bind() = 0;
};

class BasicTexture : public Texture
{
public:
    void setRepeated(bool);
    void setSmooth(bool);
    
    void loadFromImage(Image&& image);

    virtual void bind();

private:
    bool smooth = false;
    unsigned int handle = 0;
    sp::Image image;
};

}

#endif//SP_GRAPHICS_TEXTURE_H

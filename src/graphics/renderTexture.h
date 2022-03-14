#ifndef SP_GRAPHICS_RENDER_TEXTURE_H
#define SP_GRAPHICS_RENDER_TEXTURE_H

#include <graphics/texture.h>
#include <glm/vec2.hpp>


namespace sp {

class RenderTexture : public Texture
{
public:
    /**
        Create a RenderTexture.
        This can be used as both a texture, as well as a target to render on.

        @param name: Name of this render target, for debugging.
        @param size: Size of the render target in pixels.
     */
    RenderTexture(glm::ivec2 size);
    virtual ~RenderTexture();

    //Get the texture for rendering to other targets.
    virtual void bind() override;

    void setSize(glm::ivec2 size);
    glm::ivec2 getSize() const;

    //Active rendering towards this texture, returns false if rendering to texture is not supported.
    bool activateRenderTarget();
private:
    bool create();

    glm::ivec2 size;

    bool dirty = false;
    bool smooth = false;
    bool create_buffers = true;

    unsigned int frame_buffer = 0;
    unsigned int color_buffer = 0;
    unsigned int depth_buffer = 0;
    unsigned int stencil_buffer = 0;
};

}//namespace sp

#endif//RENDER_TEXTURE_H

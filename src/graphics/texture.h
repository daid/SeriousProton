#ifndef SP_GRAPHICS_TEXTURE_H
#define SP_GRAPHICS_TEXTURE_H

#include "nonCopyable.h"
#include "graphics/image.h"

namespace sp {

class Texture
{
public:
    Texture();
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    virtual ~Texture();
    virtual void bind() = 0;
};

class BasicTexture final : public Texture
{
public:
    explicit BasicTexture(const Image& image);
    BasicTexture(const glm::uvec2& size, const std::vector<uint8_t>& pixels, uint32_t native_format);
    ~BasicTexture() override;
    BasicTexture(BasicTexture&&);
    BasicTexture& operator =(BasicTexture&&);

    void setRepeated(bool);
    void setSmooth(bool);

    virtual void bind() override;

    uint32_t getNativeHandle() const { return handle; }
private:
    BasicTexture(const glm::uvec2& size, uint32_t native_format, const void* pixels, size_t byte_count);
    void setSmoothBindless(bool);
    void setRepeatedBindless(bool);
    uint32_t handle = 0;  
};

}

#endif//SP_GRAPHICS_TEXTURE_H

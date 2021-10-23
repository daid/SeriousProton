#ifndef SP_GRAPHICS_IMAGE_H
#define SP_GRAPHICS_IMAGE_H

#include "stringImproved.h"
#include "resources.h"
#include "rect.h"
#include <glm/gtc/type_precision.hpp>


namespace sp {

class Image
{
public:
    Image();
    Image(Image&& other) noexcept;
    Image(glm::ivec2 size);
    Image(glm::ivec2 size, glm::u8vec4 color);
    Image(glm::ivec2 size, std::vector<glm::u8vec4>&& pixels, uint32_t format = 0, glm::uvec2 texsize = {});
    
    void operator=(Image&& other) noexcept;
    Image(const Image&) = delete;
    Image& operator =(const Image&) = delete;
    
    void update(glm::ivec2 size, const glm::u8vec4* ptr);
    void update(glm::ivec2 size, const glm::u8vec4* ptr, int pitch);
    bool loadFromStream(P<ResourceStream> stream);
    
    glm::ivec2 getSize() const { return size; }
    const glm::u8vec4* getPtr() const { return &pixels[0]; }
    glm::u8vec4* getPtr() { return &pixels[0]; }
    uint32_t getFormat() const { return format;  }
    
    glm::vec2 tex_size{};
private:
    std::vector<glm::u8vec4> pixels;
    glm::ivec2 size;
    uint32_t format{};
};

}//namespace sp

#endif//SP_GRAPHICS_IMAGE_H

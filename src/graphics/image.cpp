#include "graphics/image.h"
#include <cassert>

#define STBI_ASSERT(x) assert(x)
#define STB_IMAGE_IMPLEMENTATION
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wshadow-compatible-local"
#endif//__GNUC__
#include "stb/stb_image.h"

namespace sp {

Image::Image()
{
}

Image::Image(Image&& other) noexcept
{
    pixels = std::move(other.pixels);
    size = other.size;
    other.size = {0, 0};
}

Image::Image(glm::ivec2 size)
: size(size)
{
    pixels.resize(size.x * size.y);
}

Image::Image(glm::ivec2 size, glm::u8vec4 color)
: size(size)
{
    pixels.resize(size.x * size.y, color);
}

Image::Image(glm::ivec2 size, std::vector<glm::u8vec4>&& pixels)
: size(size)
{
    assert(static_cast<size_t>(size.x * size.y) == pixels.size());
    this->pixels = std::move(pixels);
}

void Image::operator=(Image&& other) noexcept
{
    if (this == &other) return;
    pixels = std::move(other.pixels);
    size = other.size;
    other.size = {0, 0};
}

void Image::update(glm::ivec2 size, const glm::u8vec4* ptr)
{
    this->size = size;
    pixels.resize(size.x * size.y);
    memcpy(pixels.data(), ptr, size.x * size.y * sizeof(glm::u8vec4));
}

void Image::update(glm::ivec2 size, const glm::u8vec4* ptr, int pitch)
{
    this->size = size;
    pixels.resize(size.x * size.y);
    for(int y=0; y<size.y; y++)
        memcpy(pixels.data() + size.x * y, ptr + pitch * y, size.x * sizeof(glm::u8vec4));
}

static int stream_read(void *user, char *data, int size)
{
    ResourceStream* stream = static_cast<ResourceStream*>(user);
    return stream->read(data, size);
}

static void stream_skip(void *user, int n)
{
    ResourceStream* stream = static_cast<ResourceStream*>(user);
    stream->seek(stream->tell() + n);
}

static int stream_eof(void *user)
{
    ResourceStream* stream = static_cast<ResourceStream*>(user);
    return stream->tell() == stream->getSize();
}

static stbi_io_callbacks stream_callbacks{
    .read = stream_read,
    .skip = stream_skip,
    .eof = stream_eof,
};

bool Image::loadFromStream(P<ResourceStream> stream)
{
    if (!stream)
        return false;
    int x, y, channels;
    glm::u8vec4* buffer = reinterpret_cast<glm::u8vec4*>(stbi_load_from_callbacks(&stream_callbacks, *stream, &x, &y, &channels, 4));
    if (buffer)
    {
        update({x, y}, buffer);
        stbi_image_free(buffer);
    }
    else
    {
        return false;
    }

    return true;
}


}//namespace sp

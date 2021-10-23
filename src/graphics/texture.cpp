#include "graphics/texture.h"
#include "graphics/opengl.h"

#include <SDL_assert.h>


namespace sp {

Texture::Texture() = default;
Texture::~Texture() = default;

void BasicTexture::setRepeatedBindless(bool repeated)
{
	SDL_assert(handle != 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeated ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeated ? GL_REPEAT : GL_CLAMP_TO_EDGE);
}

void BasicTexture::setRepeated(bool repeated)
{
	glBindTexture(GL_TEXTURE_2D, handle);
	setRepeatedBindless(repeated);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);
}

void BasicTexture::setSmoothBindless(bool smooth)
{
	SDL_assert(handle != 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, smooth ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, smooth ? GL_LINEAR : GL_NEAREST);
}

void BasicTexture::setSmooth(bool smooth)
{
	SDL_assert(handle != 0);
	glBindTexture(GL_TEXTURE_2D, handle);
	setSmoothBindless(smooth);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);
}


BasicTexture::BasicTexture(const glm::uvec2& size, uint32_t native_format, const void* pixels, size_t byte_count)
{
	glGenTextures(1, &handle);
	glBindTexture(GL_TEXTURE_2D, handle);
	if (native_format == GL_RGBA)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	else
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, native_format, static_cast<GLsizei>(size.x), static_cast<GLsizei>(size.y), 0, static_cast<GLsizei>(byte_count), pixels);
	
	setRepeatedBindless(false);
	setSmoothBindless(false);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);
}

BasicTexture::BasicTexture(const glm::uvec2& size, const std::vector<uint8_t>& pixels, uint32_t native_format)
	:BasicTexture(size, native_format, pixels.data(), pixels.size())
{
}


BasicTexture::BasicTexture(const Image& image)
	:BasicTexture(image.getSize(), GL_RGBA, image.getPtr(), 0)
{
}

BasicTexture::~BasicTexture()
{
	if (handle)
		glDeleteTextures(1, &handle);
}

BasicTexture::BasicTexture(BasicTexture&& other)
	:handle{other.handle}
{
	other.handle = 0;
}

BasicTexture& BasicTexture::operator =(BasicTexture&& other)
{
	if (this != &other)
	{
		glDeleteTextures(1, &handle);
		handle = other.handle;
		other.handle = 0;
	}

	return *this;
}

void BasicTexture::bind()
{
    glBindTexture(GL_TEXTURE_2D, handle);
}
} // namespace sp

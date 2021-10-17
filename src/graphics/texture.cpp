#include "graphics/texture.h"
#include "graphics/opengl.h"

#include <array>

#define STB_DXT_IMPLEMENTATION
#define STB_DXT_STATIC
#include <cstring>
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#endif//__GNUC__
#include "stb/stb_dxt.h"
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif//__GNUC__

namespace {
	std::vector<uint8_t> compressDxt(const sp::Image &image_data)
	{
		auto width = image_data.getSize().x;
		auto height = image_data.getSize().y;
		constexpr auto channels = 4;
		// 1 channel: Grey (no alpha).
		// 2 channels: LumAlpha
		// 3 channels: RGB (no alpha!)
		// 4 channels: RGBA.
		auto block_size = channels % 2 == 0 ? 16 : 8;
		auto blocks_x = (width + 4 - 1) / 4;
		auto blocks_y = (height + 4 - 1) / 4;
		std::vector<uint8_t> compressed(static_cast<size_t>(block_size * blocks_x * blocks_y));

		auto pixels = reinterpret_cast<const uint32_t*>(image_data.getPtr());
		auto output = compressed.data();

		// Traverse each 4x4 block from the source image.
		for (auto by = 0; by < blocks_y; ++by)
		{
			auto start_y = 4 * by;
			for (auto bx = 0; bx < blocks_x; ++bx)
			{
				// Each block is 4x4, with 4 components (RGBA).
				std::array<uint32_t, 4 * 4> block{};
				// Load up the block.
				for (auto y = 0; y < 4; ++y)
					for (auto x = 0; x < 4; ++x)
					{
						auto pixel_offset = (start_y + y) * width + 4 * bx + x;
						if (pixel_offset < width * height)
							block[y * 4 + x] = pixels[pixel_offset];
					}


				stb_compress_dxt_block(output, reinterpret_cast<const uint8_t*>(block.data()), channels % 2 == 0 ? 1 : 0, STB_DXT_HIGHQUAL);
				output += block_size;
			}
		}

		return compressed;
	}
}


namespace sp {

void BasicTexture::setRepeated(bool)
{

}

void BasicTexture::setSmooth(bool value)
{
    smooth = value;
}

void BasicTexture::loadFromImage(Image&& image)
{
    this->image = std::move(image);
}

void BasicTexture::bind()
{
    if (handle == 0)
    {
        glGenTextures(1, &handle);
        glBindTexture(GL_TEXTURE_2D, handle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, smooth ? GL_LINEAR : GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, smooth ? GL_LINEAR : GL_NEAREST);

        if (GLAD_GL_EXT_texture_compression_s3tc)
        {
			auto compressed = compressDxt(image);
			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, image.getSize().x, image.getSize().y, 0, static_cast<GLsizei>(compressed.size()), compressed.data());
        }
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.getSize().x, image.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.getPtr());
		}

        image = {};
    }

    glBindTexture(GL_TEXTURE_2D, handle);
}

}

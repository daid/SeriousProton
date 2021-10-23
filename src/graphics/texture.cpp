#include "graphics/texture.h"
#include "graphics/opengl.h"

#include <array>
#include <SDL_assert.h>

#include <transcoder/basisu_transcoder.h>

namespace {
#if defined(ANDROID)
	constexpr auto forced_mip = 2;
#else
	constexpr auto forced_mip = 0;
#endif

	// one block per 4x4(=16) pixels.
	constexpr uint32_t pixels_per_block = 4 * 4;
	constexpr uint32_t getBlockCount(const glm::uvec2& size)
	{
		return (size.x * size.y + pixels_per_block - 1) / pixels_per_block;
	}

	basist::transcoder_texture_format uastcLoadFormat(const glm::uvec2& size, bool has_alpha, const std::optional<glm::uvec2>& threshold)
	{
		using format_t = basist::transcoder_texture_format;

		if (threshold.has_value())
		{
			// Do we want uncompressed (texture atlas)
			auto dims = threshold.value();
			if (size.x <= dims.x || size.y <= dims.y)
				return format_t::cTFRGBA32;
		}

		// Use GPU capabilities, in order of preference.
		// ASTC
		if (GLAD_GL_KHR_texture_compression_astc_ldr)
			return format_t::cTFASTC_4x4_RGBA; // No RGB variant.

		// ETC2
		if (SP_texture_compression_etc2)
			return format_t::cTFETC2_RGBA;

		// PVRTC2
		if (GLAD_GL_IMG_texture_compression_pvrtc2)
			return has_alpha ? format_t::cTFPVRTC2_4_RGBA : format_t::cTFPVRTC2_4_RGB;

		// PVRTC1
		if (GLAD_GL_IMG_texture_compression_pvrtc)
			return has_alpha ? format_t::cTFPVRTC1_4_RGBA : format_t::cTFPVRTC1_4_RGB;
		
		// S3TC/DXT.
		if (GLAD_GL_EXT_texture_compression_s3tc)
			return has_alpha ? format_t::cTFBC3_RGBA : format_t::cTFBC1_RGB;
		
		// ETC1.
		if (GLAD_GL_OES_compressed_ETC1_RGB8_texture && !has_alpha)
			return format_t::cTFETC1_RGB;

		return format_t::cTFRGBA32;
	}

	constexpr GLenum basistFormatCast(basist::transcoder_texture_format format)
	{
		// https://github.com/BinomialLLC/basis_universal/wiki/OpenGL-texture-format-enums-table
		using format_t = basist::transcoder_texture_format;

		switch (format)
		{
			// ASTC
		case format_t::cTFASTC_4x4_RGBA:
			return GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
			
			// ETC2 RGBA
		case format_t::cTFETC2_RGBA:
			return GL_COMPRESSED_RGBA8_ETC2_EAC;

			// PVRTC2
		case format_t::cTFPVRTC2_4_RGB:
			[[fallthrough]];
		case format_t::cTFPVRTC2_4_RGBA:
			return GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG;

			// PVRTC1
		case format_t::cTFPVRTC1_4_RGB:
			return GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
		case format_t::cTFPVRTC1_4_RGBA:
			return GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;

			// S3TC/DXT
		case format_t::cTFBC1_RGB:
			return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		case format_t::cTFBC3_RGBA:
			return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		
			// ETC1
		case format_t::cTFETC1_RGB:
			return GL_ETC1_RGB8_OES;
		
		default: // unknown?
			[[fallthrough]];

			// no compression.
		case format_t::cTFRGBA32:
			return 0;
		}
	}

	constexpr uint32_t bytesPerBlock(GLenum format)
	{
		switch (format)
		{
			// 16 B (RGBA formats)
		case GL_COMPRESSED_RGBA_ASTC_4x4_KHR: // ASTC
			[[fallthrough]];
		case GL_COMPRESSED_RGBA8_ETC2_EAC: // ETC2
			[[fallthrough]];
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT: // BC3 (RGBA)
			return 16;

			// 8 B (RGB formats)
		case GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG: // PVRTC1
			[[fallthrough]];
		case GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG: // PVRTC1 (RGBA)
			[[fallthrough]];
		case GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG: // PVRTC2
			[[fallthrough]];
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT: // BC1
			[[fallthrough]];
		case GL_ETC1_RGB8_OES: // ETC1
			return 8;

		default:
			return 0; // not a block format/unrecognized.
		}
	}
}


namespace sp {

	Image Texture::loadUASTC(const P<ResourceStream>& stream, std::optional<glm::uvec2> threshold)
	{
		static std::unique_ptr<basist::etc1_global_selector_codebook> codebook;
		if (!codebook)
		{
			basist::basisu_transcoder_init();
			codebook = std::make_unique<basist::etc1_global_selector_codebook>(basist::g_global_selector_cb_size, basist::g_global_selector_cb);
		}

		basist::ktx2_transcoder transcoder(codebook.get());
		auto data = stream->readAll();
		if (transcoder.init(data.data(), static_cast<uint32_t>(data.size())))
		{
			if (transcoder.start_transcoding())
			{
				if (basist::ktx2_image_level_info info; transcoder.get_image_level_info(info, forced_mip, 0, 0))
				{
					const auto format = uastcLoadFormat({ info.m_orig_width , info.m_orig_height }, info.m_alpha_flag, threshold);

					std::vector<glm::u8vec4> pixels;
					if (basist::basis_transcoder_format_is_uncompressed(format))
						pixels.resize(info.m_orig_width * info.m_orig_height);
					else
					{
						auto block_size = basist::basis_get_bytes_per_block_or_pixel(format);

						// each item in the pixels array is 4B.
						pixels.resize(info.m_total_blocks * bytesPerBlock(basistFormatCast(format)) / 4);
					}

					if (transcoder.transcode_image_level(forced_mip, 0, 0, pixels.data(), static_cast<uint32_t>(pixels.size() * 4), format))
					{
						glm::ivec2 dims{ static_cast<int32_t>(info.m_orig_width), static_cast<int32_t>(info.m_orig_height) };
						auto gl_format = basistFormatCast(format);
						return Image(dims, std::move(pixels), gl_format, { info.m_width, info.m_height });
					}
				}
			}
		}

		return {};
	}

	size_t Texture::compressedSize(const Image& image)
	{
		if (image.getFormat() == 0) // not compressed
			return 0;
		return getBlockCount(image.getSize()) * bytesPerBlock(image.getFormat());
	}

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

		if (image.getFormat() != 0)
		{
			const auto compressed_size = getBlockCount(image.tex_size) * bytesPerBlock(image.getFormat());
			SDL_assert_always(compressed_size > 0);
			SDL_assert_always(image.tex_size.x > 0);
			SDL_assert_always(image.tex_size.y > 0);
			glCompressedTexImage2D(GL_TEXTURE_2D, 0, image.getFormat(), static_cast<GLsizei>(image.tex_size.x), static_cast<GLsizei>(image.tex_size.y), 0, static_cast<GLsizei>(compressed_size), image.getPtr());
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

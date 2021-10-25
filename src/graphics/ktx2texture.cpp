#include "graphics/ktx2texture.h"

#include <array>

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif//__GNUC__
#include <transcoder/basisu_transcoder.h>
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif//__GNUC__

#include "graphics/opengl.h"
#include "graphics/texture.h"

namespace {
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
			return GL_RGBA;
		}
	}
}

namespace sp {
	class KTX2Texture::Details final
	{
	public:
		static std::unique_ptr<Details> load(std::vector<uint8_t>&& data)
		{
			if (!codebook)
			{
				basist::basisu_transcoder_init();
				codebook = std::make_unique<basist::etc1_global_selector_codebook>(basist::g_global_selector_cb_size, basist::g_global_selector_cb);
			}

			std::unique_ptr<Details> result{ new Details{} };

			if (!result->transcoder.init(data.data(), static_cast<uint32_t>(data.size())))
				return {};

			if (result->transcoder.get_has_alpha())
				result->best_format = getBestFormatAlpha(result->transcoder.get_format());
			else
				result->best_format = getBestFormatOpaque(result->transcoder.get_format());

			result->transcoding_data = std::move(data);
			return result;
		}

		template<typename T>
		std::vector<T> transcode(uint32_t mip_level, basist::transcoder_texture_format target_format)
		{
			basist::ktx2_image_level_info info;
			if (!transcoder.get_image_level_info(info, mip_level, 0, 0))
				return {};

			if (!transcoder.start_transcoding())
				return {};

			const auto block_or_pixel_size = basist::basis_get_bytes_per_block_or_pixel(target_format);
			const auto is_uncompressed = basist::basis_transcoder_format_is_uncompressed(target_format);
			auto total_blocks_or_pixels = is_uncompressed ? info.m_orig_width * info.m_orig_height : info.m_total_blocks;

			std::vector<T> pixels_or_blocks(total_blocks_or_pixels * block_or_pixel_size / sizeof(T));

			if (!transcoder.transcode_image_level(info.m_level_index, 0, 0, pixels_or_blocks.data(), static_cast<uint32_t>(pixels_or_blocks.size() * sizeof(T)), target_format))
				return {};

			return pixels_or_blocks;
		}

		template<typename T>
		auto transcode(uint32_t mip_level)
		{
			return transcode<T>(mip_level, best_format);
		}

		~Details()
		{
			transcoder.clear();
		}

		basist::transcoder_texture_format getBestFormat() const { return best_format; }
		glm::ivec2 getSize(uint32_t mip_level) const
		{
			basist::ktx2_image_level_info info;
			if (!transcoder.get_image_level_info(info, mip_level, 0, 0))
				return {};

			return glm::ivec2{ info.m_width, info.m_height };
		}

		uint32_t getMipCount() const
		{
			return transcoder.get_levels();
		}
	private:
		static std::unique_ptr<basist::etc1_global_selector_codebook> codebook;

		// Use GPU capabilities, in order of preference.
		static basist::transcoder_texture_format getBestFormatAlpha(basist::basis_tex_format tex_format)
		{
			using format_t = basist::transcoder_texture_format;

			const std::array possible_formats{
				// ASTC
				std::tuple{GLAD_GL_KHR_texture_compression_astc_ldr, format_t::cTFASTC_4x4_RGBA},
				// ETC2
				std::tuple{SP_texture_compression_etc2, format_t::cTFETC2_RGBA},
				// PVRTC2
				std::tuple{GLAD_GL_IMG_texture_compression_pvrtc2, format_t::cTFPVRTC2_4_RGBA},
				// PVRTC1
				std::tuple{GLAD_GL_IMG_texture_compression_pvrtc, format_t::cTFPVRTC1_4_RGBA},
				// S3TC DXT5A (BC3)
				std::tuple{GLAD_GL_EXT_texture_compression_s3tc, format_t::cTFBC3_RGBA}
			};

			for (const auto& [available, target_format] : possible_formats)
			{
				if (available && basist::basis_is_format_supported(target_format, tex_format))
					return target_format;
			}

			return format_t::cTFRGBA32;
		}

		static basist::transcoder_texture_format getBestFormatOpaque(basist::basis_tex_format tex_format)
		{
			using format_t = basist::transcoder_texture_format;

			const std::array possible_formats{
				// ASTC
				std::tuple{GLAD_GL_KHR_texture_compression_astc_ldr, format_t::cTFASTC_4x4_RGBA},
				// ETC2
				std::tuple{SP_texture_compression_etc2, format_t::cTFETC2_RGBA},
				// PVRTC2
				std::tuple{GLAD_GL_IMG_texture_compression_pvrtc2, format_t::cTFPVRTC2_4_RGB},
				// PVRTC1
				std::tuple{GLAD_GL_IMG_texture_compression_pvrtc, format_t::cTFPVRTC1_4_RGB},
				// S3TC DXT1 (BC1)
				std::tuple{GLAD_GL_EXT_texture_compression_s3tc, format_t::cTFBC1_RGB},
				// ETC1
				std::tuple{GLAD_GL_OES_compressed_ETC1_RGB8_texture, format_t::cTFETC1_RGB},
			};

			for (const auto& [available, target_format]: possible_formats)
			{
				if (available && basist::basis_is_format_supported(target_format, tex_format))
					return target_format;
			}

			return format_t::cTFRGBA32;
		}

		Details()
			:transcoder{codebook.get()}
		{
		}

		basist::ktx2_transcoder transcoder;
		std::vector<uint8_t> transcoding_data;
		basist::transcoder_texture_format best_format{ basist::transcoder_texture_format::cTFRGBA32 };
	};

	std::unique_ptr<basist::etc1_global_selector_codebook> KTX2Texture::Details::codebook;

	bool KTX2Texture::loadFromStream(P<ResourceStream> stream)
	{
		if (!stream)
			return false;

		std::vector<uint8_t> data(stream->getSize());
		stream->read(data.data(), data.size());
		details = Details::load(std::move(data));

		return details != nullptr;
	}

	std::optional<Image> KTX2Texture::toImage(uint32_t mip_level)
	{
		if (!details)
			return {};

		auto pixels = details->transcode<glm::u8vec4>(mip_level, basist::transcoder_texture_format::cTFRGBA32);
		if (pixels.empty())
			return {};

		return std::make_optional<Image>(getSize(mip_level), std::move(pixels));
	}

	std::unique_ptr<BasicTexture> KTX2Texture::toTexture(uint32_t mip_level)
	{
		if (!details)
			return {};
		
		if (auto pixels = toNative(mip_level); !pixels.empty())
			return std::make_unique<BasicTexture>(getSize(mip_level), std::move(pixels), getNativeFormat());
		
		return {};
	}

	std::vector<uint8_t> KTX2Texture::toNative(uint32_t mip_level)
	{
		if (!details)
			return {};

		return details->transcode<uint8_t>(mip_level);
	}

	glm::ivec2 KTX2Texture::getSize(uint32_t mip_level) const
	{
		return details ? details->getSize(mip_level) : glm::ivec2{};
	}

	uint32_t KTX2Texture::getNativeFormat() const
	{
		return details ? basistFormatCast(details->getBestFormat()) : GL_NONE;
	}

	uint32_t KTX2Texture::getMipCount() const
	{
		if (!details)
			return 0;
		return details->getMipCount();
	}

	KTX2Texture::KTX2Texture() = default;
	KTX2Texture::~KTX2Texture() = default;
	KTX2Texture::KTX2Texture(KTX2Texture&&) = default;
	KTX2Texture& KTX2Texture::operator =(KTX2Texture&&) = default;
} // namespace sp

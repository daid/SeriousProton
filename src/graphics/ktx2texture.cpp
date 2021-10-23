#include "graphics/ktx2texture.h"

#include <array>

#include <transcoder/basisu_transcoder.h>

#include "graphics/opengl.h"
#include "graphics/texture.h"

namespace {
#if defined(ANDROID)
	constexpr auto forced_mip = 2;
#else
	constexpr auto forced_mip = 0;
#endif

	constexpr basist::transcoder_texture_format getBestFormat(bool has_alpha)
	{
		using format_t = basist::transcoder_texture_format;

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

			if (!result->transcoder.get_image_level_info(result->info, forced_mip, 0, 0))
				return {};	

			result->transcoding_data = std::move(data);
			return result;
		}


		template<typename T>
		std::vector<T> transcode(basist::transcoder_texture_format target_format)
		{
			if (!transcoder.start_transcoding())
				return {};

			const auto block_or_pixel_size = basist::basis_get_bytes_per_block_or_pixel(target_format);
			const auto is_uncompressed = basist::basis_transcoder_format_is_uncompressed(target_format);
			auto total_blocks_or_pixels = is_uncompressed ? info.m_orig_width * info.m_orig_height : info.m_total_blocks;

			std::vector<T> pixels_or_blocks(total_blocks_or_pixels * block_or_pixel_size / sizeof(T));

			if (!transcoder.transcode_image_level(forced_mip, 0, 0, pixels_or_blocks.data(), static_cast<uint32_t>(pixels_or_blocks.size() * sizeof(T)), target_format))
				return {};

			return pixels_or_blocks;
		}

		~Details()
		{
			transcoder.clear();
		}

		const auto& getInfo() const { return info; }
	private:
		static std::unique_ptr<basist::etc1_global_selector_codebook> codebook;

		Details()
			:transcoder{codebook.get()}
		{
		}

		basist::ktx2_transcoder transcoder;
		basist::ktx2_image_level_info info;
		std::vector<uint8_t> transcoding_data;
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

	std::optional<Image> KTX2Texture::toImage()
	{
		if (!details)
			return {};

		auto pixels = details->transcode<glm::u8vec4>(basist::transcoder_texture_format::cTFRGBA32);
		if (pixels.empty())
			return {};
		const auto size = getSize();
		return std::make_optional<Image>(size, std::move(pixels));
	}

	std::unique_ptr<BasicTexture> KTX2Texture::toTexture()
	{
		if (!details)
			return {};

		auto target_format = getBestFormat(details->getInfo().m_alpha_flag);
		if (basist::basis_transcoder_format_is_uncompressed(target_format))
		{
			if (auto image = toImage(); image.has_value())
				return std::make_unique<BasicTexture>(image.value());
		}
		else
		{
			if (auto pixels = toNative(); !pixels.empty())
				return std::make_unique<BasicTexture>(getSize(), std::move(pixels), getNativeFormat());
		}
		
		return {};
	}

	std::vector<uint8_t> KTX2Texture::toNative()
	{
		if (!details)
			return {};

		return details->transcode<uint8_t>(getBestFormat(details->getInfo().m_alpha_flag));
	}

	glm::ivec2 KTX2Texture::getSize() const
	{
		return details ? glm::ivec2(details->getInfo().m_orig_width, details->getInfo().m_orig_height) : glm::ivec2{};
	}

	uint32_t KTX2Texture::getNativeFormat() const
	{
		return details ? basistFormatCast(getBestFormat(details->getInfo().m_alpha_flag)) : GL_NONE;
	}

	KTX2Texture::KTX2Texture() = default;
	KTX2Texture::~KTX2Texture() = default;
	KTX2Texture::KTX2Texture(KTX2Texture&&) = default;
	KTX2Texture& KTX2Texture::operator =(KTX2Texture&&) = default;
} // namespace sp

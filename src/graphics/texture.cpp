#include "graphics/texture.h"
#include "graphics/opengl.h"

#include <array>
#include <cstring>

#include <astcenc.h>
#include <SDL_assert.h>
#include <atomic>
#include <thread>

#include "logging.h"

#define STB_DXT_IMPLEMENTATION
#define STB_DXT_STATIC
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

    class AstcCompressor final
    {
    public:
        static std::unique_ptr<AstcCompressor> load()
        {
            std::unique_ptr<AstcCompressor> compressor{ new AstcCompressor };

            auto status = astcenc_config_init(ASTCENC_PRF_LDR, 4, 4, 1, ASTCENC_PRE_FASTEST, ASTCENC_FLG_SELF_DECOMPRESS_ONLY, &compressor->config);
            if (status != ASTCENC_SUCCESS)
            {
                LOG(ERROR, "astcenc config init: ", astcenc_get_error_string(status));
                return {};
            }

            // According to https://en.cppreference.com/w/cpp/thread/thread/hardware_concurrency
            // Make sure we account for that edge case.
            compressor->concurrency = std::max(1u, std::thread::hardware_concurrency());
            status = astcenc_context_alloc(&compressor->config, compressor->concurrency, &compressor->context);
            if (status != ASTCENC_SUCCESS)
            {
                LOG(ERROR, "astcenc context creation: %s", astcenc_get_error_string(status));
                return {};
            }

            return compressor;
        }

        std::vector<uint8_t> compress(sp::Image& image)
        {
            const auto width = image.getSize().x;
            const auto height = image.getSize().y;
            constexpr auto channels = 4u;

            std::array<void*, 1> data{ image.getPtr() };
            astcenc_image raw
            {
                static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1u,
                ASTCENC_TYPE_U8, data.data()
            };

            std::vector<uint8_t> compressed(((raw.dim_x + 4 - 1) / 4) * ((raw.dim_y + 4 - 1) / 4) * 16);
            const auto swizzle = [](uint32_t channels) -> astcenc_swizzle
            {
                switch (channels)
                {
                case 1: // Grey
                    return { ASTCENC_SWZ_R, ASTCENC_SWZ_R, ASTCENC_SWZ_R, ASTCENC_SWZ_1 };

                case 2: // Lum Alpha
                    return { ASTCENC_SWZ_R, ASTCENC_SWZ_R, ASTCENC_SWZ_R, ASTCENC_SWZ_A };

                case 3:
                    return { ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_1 };

                case 4:
                    return { ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A };
                default:
                    SDL_assert(false); // wat
                    return { ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A };
                }
            }(channels);

            // Create worker pool, based on available threads.
            std::vector<std::thread> workers(concurrency - 1);

            // Gather errors.
            std::vector<std::atomic<astcenc_error>> errors(concurrency);

            // Worker function.
            // All the heavy synchronizing stuff is done by astcenc itself, we only set up the worker threads.
            auto worker_function = [this, &raw, &swizzle, &compressed, &errors](auto index)
            {
                errors[index] = astcenc_compress_image(context, &raw, &swizzle, compressed.data(), compressed.size(), index);
            };

            // Create and start workers...
            for (auto i = 0u; i < workers.size(); ++i)
            {
                workers[i] = std::thread(worker_function, i + 1);
            }

            // This thread always acts likes the first one
            // (covers the edge case where `hardware_concurrency()` might return 0,
            // meaning we should probably not rely on threads.
            worker_function(0);

            // ... Wait until they're all done
            for (auto& worker : workers)
            {
                worker.join();
            }

            // Check for errors (no concurrency)
            for (const auto& error : errors)
            {
                if (error != ASTCENC_SUCCESS)
                {
                    LOG(ERROR, "astcenc compression: %s", astcenc_get_error_string(error));
                    compressed.clear();
                }
            }
            
            astcenc_compress_reset(context);

            return compressed;
        }

        ~AstcCompressor()
        {
            astcenc_context_free(context);
        }

        AstcCompressor(const AstcCompressor&) = delete;
        AstcCompressor& operator=(const AstcCompressor&) = delete;
        AstcCompressor(const AstcCompressor&&) = delete;
        AstcCompressor& operator=(AstcCompressor&&) = delete;
    private:
        AstcCompressor() = default;
        
        astcenc_config config{};
        astcenc_context* context{};
        uint32_t concurrency{};
    };

    std::vector<uint8_t> compressAstc(sp::Image& image)
    {
        static auto compressor = AstcCompressor::load();
        if (!compressor)
            return {};

        return compressor->compress(image);
        
    }
} // anonymous namespace

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

        std::vector<uint8_t> compressed;
        GLenum compressed_format{ GL_NONE };
        if (GLAD_GL_KHR_texture_compression_astc_ldr)
        {
            compressed = compressAstc(image);
            compressed_format = GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
            LOG(DEBUG, "Loaded texture through astc.");
        }

        if (compressed.empty() && GLAD_GL_EXT_texture_compression_s3tc)
        {
            compressed = compressDxt(image);
            compressed_format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            LOG(DEBUG, "Loaded texture through DXT5.");
        }

        if (!compressed.empty())
        {
            glCompressedTexImage2D(GL_TEXTURE_2D, 0, compressed_format, image.getSize().x, image.getSize().y, 0, static_cast<GLsizei>(compressed.size()), compressed.data());
        }
        else
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.getSize().x, image.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.getPtr());

        image = {};
    }

    glBindTexture(GL_TEXTURE_2D, handle);
}

}

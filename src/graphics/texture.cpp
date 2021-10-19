#include "graphics/texture.h"
#include "graphics/opengl.h"

#include <array>
#include <cstring>
#include <thread>
#include <optional>

#include <SDL_assert.h>

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

#include "rg_etc1/rg_etc1.h"


namespace {
    std::vector<uint8_t> compressDxt(const sp::Image& image_data)
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

    namespace etc1 {
        // Shared data for compression.
        // All fields read-only.
        struct Shared final
        {
            static constexpr uint32_t block_size = 8; // ETC1 is 8-byte (no alpha!)

            explicit Shared(const sp::Image& image)
                : width{ static_cast<uint32_t>(image.getSize().x) }
                , height{ static_cast<uint32_t>(image.getSize().y) }
                // Compute concurrency.
                // It can be that `hardware_concurrency()` be zero, so make sure we account for it.
                // (we're running this code, so we know _something_ is running....).
                // To avoid corner cases (work starvation), we will never queue more than `HeightBlockCount()`.
                , concurrency{ std::max(1u, std::min(std::thread::hardware_concurrency(), getHeightBlockCount())) }
                , pixels{ reinterpret_cast<const uint32_t*>(image.getPtr()) }
            {}

            constexpr uint32_t getWidthBlockCount() const
            {
                return (width + 4 - 1) / 4;
            }

            constexpr uint32_t getHeightBlockCount() const
            {
                return (height + 4 - 1) / 4;
            }

            constexpr uint32_t getBlockCount() const
            {
                return getWidthBlockCount() * getHeightBlockCount();
            }

            std::tuple<uint32_t, uint32_t> getSliceBlockRange(uint32_t index) const
            {
                SDL_assert(index < concurrency);
                SDL_assert(!empty());
                const auto block_count = getHeightBlockCount();
                const auto workload_size = (block_count + concurrency - 1) / concurrency;
                auto range_start = index * workload_size;
                // By design, we don't have _more_ concurrency than we have blocks to process.
                // (ie if the system has 8 threads but we only have 7 blocks, we will only use 7 threads).
                // The only case we need to worry about is the last thread having slightly less to process.
                // For instance, 7 slices on 4 threads - each thread gets 2 except the last one, which gets the remainder (1).
                auto range_end = std::min(range_start + workload_size, block_count);
                return  { range_start, range_end };
            }

            constexpr uint32_t getBlockRangeByteSize(uint32_t range_start, uint32_t range_end) const
            {
                return (range_end - range_start) * getWidthBlockCount() * Shared::block_size;
            }

            constexpr bool empty() const
            {
                return getBlockCount() == 0;
            }

            uint32_t width{};
            uint32_t height{};
            uint32_t concurrency{};
            const uint32_t* pixels{};
        };

        std::optional<std::vector<uint8_t>> compressSlice(const Shared& shared, uint32_t slice_index)
        {
            rg_etc1::etc1_pack_params params;
            params.m_quality = rg_etc1::cLowQuality;
            params.m_dithering = false;

            auto [start_block_y, end_block_y] = shared.getSliceBlockRange(slice_index);
            auto range_size = (end_block_y - start_block_y);
            SDL_assert(range_size > 0);

            std::vector<uint8_t> compressed(shared.getBlockRangeByteSize(start_block_y, end_block_y));
            auto output = compressed.data();
            // Traverse each 4x4 block from the source image.
            for (auto by = start_block_y; by < end_block_y; ++by)
            {
                auto start_y = 4 * by;
                for (auto bx = 0u; bx < shared.getWidthBlockCount(); ++bx)
                {
                    // Each block is 4x4, with 4 components (RGBA).
                    std::array<uint32_t, 4 * 4> block{};
                    // Load up the block.
                    for (auto y = 0; y < 4; ++y)
                        for (auto x = 0; x < 4; ++x)
                        {
                            auto pixel_offset = (start_y + y) * shared.width + 4 * bx + x;
                            if (pixel_offset < shared.width * shared.height)
                            {
                                // abort if we encounter any transparency (not handled by ETC1)
                                if (reinterpret_cast<const uint8_t*>(shared.pixels + pixel_offset)[3] != 255)
                                {
                                    return {};
                                }

                                block[y * 4 + x] = shared.pixels[pixel_offset];
                            }
                        }


                    rg_etc1::pack_etc1_block(output, block.data(), params);
                    output += shared.block_size;
                }
            }

            return compressed;
        }

        std::vector<uint8_t> compress(const sp::Image& image_data)
        {
            static auto inited = false;
            if (!inited)
            {
                rg_etc1::pack_etc1_block_init();
                inited = true;
            }


            // setup shared data (read-only)
            const Shared shared{ image_data };

            if (shared.empty())
            {
                LOG(WARNING, "ETC1: Image has at least one empty dimension.");
                return {};
            }
                

            SDL_assert(shared.concurrency >= 1u);

            // Each helper writes to a 'slice', and they're saved here.
            std::vector<std::optional<std::vector<uint8_t>>> slices(shared.concurrency);


            // Threaded part.
            {
                // Create helpers.
                // Helpers exclude the thread currently running this code.
                std::vector<std::thread> helpers(shared.concurrency - 1);

                auto helper_function = [&shared](uint32_t slice_index, std::optional<std::vector<uint8_t>>& slice)
                {
                    slice = compressSlice(shared, slice_index);
                };


                LOG(DEBUG, "ETC1: Compressing with %zu threads.", helpers.size() + 1);
                // Queue additional helpers.
                for (auto i = 0u; i < helpers.size(); ++i)
                {
                    auto slice_index = i + 1;
                    helpers[i] = std::thread(helper_function, slice_index, std::ref(slices[slice_index]));
                }

                // This thread acts like the first helper.
                helper_function(0, slices[0]);

                // Wait for everyone to be done.
                for (auto& helper : helpers)
                {
                    helper.join();
                }

                // Done!
            }
            
            // Gather all the slices into one.
            std::vector<uint8_t> compressed(static_cast<size_t>(shared.getBlockRangeByteSize(0, shared.getBlockCount())));
            size_t copied{};
            for (const auto& slice : slices)
            {
                if (!slice.has_value())
                {
                    LOG(WARNING, "ETC1: aborted due to transparent pixel.");
                    return {};
                }

                const auto& data = slice.value();
                memcpy(compressed.data() + copied, data.data(), data.size());
                copied += data.size();
            }

            return compressed;
        }
    } // namespace etc1
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

        std::vector<uint8_t> compressed;
        auto compressed_type = GL_NONE;

        if (image.getSize().x > 0 && image.getSize().y > 0)
        {
            if (GLAD_GL_EXT_texture_compression_s3tc)
            {
                compressed = compressDxt(image);
                compressed_type = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;

            }

            if (compressed.empty() && GLAD_GL_OES_compressed_ETC1_RGB8_texture)
            {
                LOG(DEBUG, "Attempting ETC1 texture compression.");
                compressed = etc1::compress(image);
                compressed_type = GL_ETC1_RGB8_OES;
            }
        }

        if (!compressed.empty())
        {
            glCompressedTexImage2D(GL_TEXTURE_2D, 0, compressed_type, image.getSize().x, image.getSize().y, 0, static_cast<GLsizei>(compressed.size()), compressed.data());
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

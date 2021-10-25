#ifndef SP_GRAPHICS_KTX2TEXTURE_H
#define SP_GRAPHICS_KTX2TEXTURE_H

#include <memory>
#include <optional>

#include <glm/vec2.hpp>

#include "graphics/image.h"
#include "resources.h"

namespace sp {
	class BasicTexture;

	class KTX2Texture
	{
	public:
		bool loadFromStream(P<ResourceStream> stream);

		std::optional<Image> toImage(uint32_t mip_level = 0);
		std::unique_ptr<BasicTexture> toTexture(uint32_t mip_level = 0);
		std::vector<uint8_t> toNative(uint32_t mip_level = 0);

		glm::ivec2 getSize(uint32_t mip_level = 0) const;
		uint32_t getNativeFormat() const;
		uint32_t getMipCount() const;

		KTX2Texture();
		~KTX2Texture();
		
		// Transferrable, non-copyable.
		KTX2Texture(KTX2Texture&&);
		KTX2Texture& operator =(KTX2Texture&&);
		KTX2Texture(const KTX2Texture&) = delete;
		KTX2Texture& operator =(const KTX2Texture&) = delete;
	private:
		class Details;
		std::unique_ptr<Details> details;
	};
}
#endif // SP_GRAPHICS_KTX2TEXTURE_H

#ifndef SP_GRAPHICS_RENDERTARGET_H
#define SP_GRAPHICS_RENDERTARGET_H

#include <glm/vec2.hpp>
#include <glm/gtc/type_precision.hpp>
#include <string_view>
#include <vector>
#include "nonCopyable.h"
#include "rect.h"
#include "graphics/font.h"
#include "graphics/alignment.h"


class Window;
namespace sp {
class Texture;

class RenderTarget : sp::NonCopyable
{
private:
    RenderTarget(glm::vec2 virtual_size, glm::ivec2 physical_size);
    void finish(sp::Texture* texture);

    friend class ::Window;
public:
    static void setDefaultFont(sp::Font* font);
    static sp::Font* getDefaultFont();

    void drawSprite(std::string_view texture, glm::vec2 center, float size, glm::u8vec4 color={255,255,255,255});
    void drawSpriteClipped(std::string_view texture, glm::vec2 center, float size, sp::Rect clip_rect, glm::u8vec4 color={255,255,255,255});
    void drawRotatedSprite(std::string_view texture, glm::vec2 center, float size, float rotation, glm::u8vec4 color={255,255,255,255});
    void drawRotatedSpriteBlendAdd(std::string_view texture, glm::vec2 center, float size, float rotation);

    void drawLine(glm::vec2 start, glm::vec2 end, glm::u8vec4 color);
    void drawLine(glm::vec2 start, glm::vec2 end, glm::u8vec4 start_color, glm::u8vec4 end_color);
    void drawLine(const std::initializer_list<glm::vec2>& points, glm::u8vec4 color);
    void drawLine(const std::vector<glm::vec2>& points, glm::u8vec4 color);
    void drawLineBlendAdd(const std::vector<glm::vec2>& points, glm::u8vec4 color);
    void drawPoint(glm::vec2 position, glm::u8vec4 color);
    void drawRectColorMultiply(const sp::Rect& rect, glm::u8vec4 color);
    void drawCircleOutline(glm::vec2 center, float radius, float thickness, glm::u8vec4 color);
    void drawTiled(const sp::Rect& rect, std::string_view texture, glm::vec2 offset={0,0});
    void drawTriangleStrip(const std::initializer_list<glm::vec2>& points, glm::u8vec4 color);
    void drawTriangleStrip(const std::vector<glm::vec2>& points, glm::u8vec4 color);
    void drawTriangles(const std::vector<glm::vec2>& points, const std::vector<uint16_t>& indices, glm::u8vec4 color);
    void fillCircle(glm::vec2 center, float radius, glm::u8vec4 color);
    void outlineRect(const sp::Rect& rect, glm::u8vec4 color);
    void fillRect(const sp::Rect& rect, glm::u8vec4 color);

    void drawTexturedQuad(std::string_view texture,
        glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, glm::vec2 p3,
        glm::vec2 uv0, glm::vec2 uv1, glm::vec2 uv2, glm::vec2 uv3,
        glm::u8vec4 color);

    /*!
     * Draw a certain text on the screen with horizontal orientation.
     * \param rect Area to draw in
     * \param align Alighment of text.
     * \param text_size Size of the text
     * \param color Color of text
     */
    void drawText(sp::Rect rect, std::string_view text, Alignment align = Alignment::TopLeft, float text_size = 30, sp::Font* font = nullptr, glm::u8vec4 color={255,255,255,255}, int flags=0);
    void drawText(sp::Rect rect, const sp::Font::PreparedFontString& prepared, int flags=0);
    void drawRotatedText(glm::vec2 center, float rotation, std::string_view text, float font_size, sp::Font* font, glm::u8vec4 color);

    void drawStretched(sp::Rect rect, std::string_view texture, glm::u8vec4 color={255,255,255,255});
    void drawStretchedH(sp::Rect rect, std::string_view texture, glm::u8vec4 color={255,255,255,255});
    void drawStretchedV(sp::Rect rect, std::string_view texture, glm::u8vec4 color={255,255,255,255});
    void drawStretchedHV(sp::Rect rect, float corner_size, std::string_view texture, glm::u8vec4 color={255,255,255,255});
    void drawStretchedHVClipped(sp::Rect rect, sp::Rect clip_rect, float corner_size, std::string_view texture, glm::u8vec4 color={255,255,255,255});

    void finish();
    struct VertexData
    {
        glm::vec2 position;
        glm::u8vec4 color;
        glm::vec2 uv;
    };

    void applyBuffer(sp::Texture* texture, std::vector<VertexData> &data, std::vector<uint16_t> &index, int mode);

    glm::vec2 getVirtualSize();
    glm::ivec2 getPhysicalSize(); //Size in pixels
    glm::ivec2 virtualToPixelPosition(glm::vec2);

private:
    glm::vec2 virtual_size;
    glm::ivec2 physical_size;
};

}

#endif//SP_GRAPHICS_RENDERTARGET_H

#ifndef SP_GRAPHICS_RENDERTARGET_H
#define SP_GRAPHICS_RENDERTARGET_H

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Color.hpp>
#include <glm/vec2.hpp>
#include <glm/gtc/type_precision.hpp>
#include <string_view>
#include "nonCopyable.h"
#include "rect.h"

namespace sp {

enum class Alignment
{
    TopLeft,
    TopRight,
    TopCenter,
    CenterLeft,
    CenterRight,
    Center,
    BottomLeft,
    BottomRight,
    BottomCenter
};

class RenderTarget : sp::NonCopyable
{
public:
    RenderTarget(sf::RenderTarget& target);

    static void setDefaultFont(sf::Font* font);

    void drawSprite(std::string_view texture, glm::vec2 center, float size, glm::u8vec4 color={255,255,255,255});
    void drawRotatedSprite(std::string_view texture, glm::vec2 center, float size, float rotation, glm::u8vec4 color={255,255,255,255});
    void drawRotatedSpriteBlendAdd(std::string_view texture, glm::vec2 center, float size, float rotation);

    void drawLine(glm::vec2 start, glm::vec2 end, glm::u8vec4 color);
    void drawLine(glm::vec2 start, glm::vec2 end, glm::u8vec4 start_color, glm::u8vec4 end_color);
    void drawLine(const std::initializer_list<glm::vec2> points, glm::u8vec4 color);
    void drawLine(const std::vector<glm::vec2> points, glm::u8vec4 color);
    void drawLineBlendAdd(const std::vector<glm::vec2> points, glm::u8vec4 color);
    void drawPoint(glm::vec2 position, glm::u8vec4 color);
    void drawRectColorMultiply(const sp::Rect& rect, glm::u8vec4 color);
    void drawCircleOutline(glm::vec2 center, float radius, float thickness, glm::u8vec4 color);
    void drawTiled(const sp::Rect& rect, std::string_view texture);    //TODO this draws pixel perfect texture size, unlike all other functions.
    void drawTriangleStrip(const std::initializer_list<glm::vec2>& points, glm::u8vec4 color);
    void drawTriangleStrip(const std::vector<glm::vec2>& points, glm::u8vec4 color);
    void fillCircle(glm::vec2 center, float radius, glm::u8vec4 color);
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
    void drawText(sp::Rect rect, std::string_view text, Alignment align = Alignment::TopLeft, float text_size = 30, sf::Font* font = nullptr, glm::u8vec4 color={255,255,255,255});

    /*!
     * Draw a certain text on the screen with vertical orientation
     * \param rect Area to draw in
     * \param align Alighment of text.
     * \param text_size Size of the text
     * \param color Color of text
     */
    void drawVerticalText(sp::Rect rect, std::string_view text, Alignment align = Alignment::TopLeft, float text_size = 30, sf::Font* font = nullptr, glm::u8vec4 color={255,255,255,255});

    void draw9Cut(sp::Rect rect, std::string_view texture, glm::u8vec4 color={255,255,255,255}, float width_factor = 1.0);
    void draw9CutV(sp::Rect rect, std::string_view texture, glm::u8vec4 color={255,255,255,255}, float height_factor = 1.0);

    void drawStretched(sp::Rect rect, std::string_view texture, glm::u8vec4 color={255,255,255,255});
    void drawStretchedH(sp::Rect rect, std::string_view texture, glm::u8vec4 color={255,255,255,255});
    void drawStretchedV(sp::Rect rect, std::string_view texture, glm::u8vec4 color={255,255,255,255});
    void drawStretchedHV(sp::Rect rect, float corner_size, std::string_view texture, glm::u8vec4 color={255,255,255,255});

    glm::vec2 getVirtualSize() { return glm::vec2{target.getView().getSize().x, target.getView().getSize().y}; }

    //For now, provide some way to access the SFML render target, we want to get rid of this.
    // But in some locations it's a complex change.
    sf::RenderTarget& getSFMLTarget() { return target; }
private:
    sf::RenderTarget& target;
};

}

#endif//SP_GRAPHICS_RENDERTARGET_H

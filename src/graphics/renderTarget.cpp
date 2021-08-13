#include "graphics/renderTarget.h"
#include <SFML/Graphics.hpp>
#include "textureManager.h"
#include "windowManager.h"
#include "engine.h"


namespace sp {

static sf::Font* default_font;

RenderTarget::RenderTarget(sf::RenderTarget& target)
: target(target)
{
}

void RenderTarget::setDefaultFont(sf::Font* font)
{
    default_font = font;
}

void RenderTarget::drawSprite(std::string_view texture, glm::vec2 center, float size, glm::u8vec4 color)
{
    sf::Sprite sprite;
    textureManager.setTexture(sprite, texture);
    sprite.setPosition(sf::Vector2f(center.x, center.y));
    sprite.setScale(size / sprite.getTextureRect().height, size / sprite.getTextureRect().height);
    sprite.setColor(sf::Color(color.r, color.g, color.b, color.a));
    target.draw(sprite);
}

void RenderTarget::drawRotatedSprite(std::string_view texture, glm::vec2 center, float size, float rotation, glm::u8vec4 color)
{
    sf::Sprite sprite;
    textureManager.setTexture(sprite, texture);
    sprite.setPosition(sf::Vector2f(center.x, center.y));
    sprite.setScale(size / sprite.getTextureRect().height, size / sprite.getTextureRect().height);
    sprite.setColor(sf::Color(color.r, color.g, color.b, color.a));
    sprite.setRotation(rotation);
    target.draw(sprite);
}

void RenderTarget::drawRotatedSpriteBlendAdd(std::string_view texture, glm::vec2 center, float size, float rotation)
{
    sf::Sprite sprite;
    textureManager.setTexture(sprite, texture);
    sprite.setPosition(sf::Vector2f(center.x, center.y));
    sprite.setScale(size / sprite.getTextureRect().height, size / sprite.getTextureRect().height);
    sprite.setRotation(rotation);
    target.draw(sprite, sf::BlendAdd);
}

void RenderTarget::drawLine(glm::vec2 start, glm::vec2 end, glm::u8vec4 color)
{
    sf::VertexArray a(sf::LinesStrip, 2);
    a[0].position.x = start.x;
    a[0].position.y = start.y;
    a[1].position.x = end.x;
    a[1].position.y = end.y;
    a[0].color = sf::Color(color.r, color.g, color.b, color.a);
    a[1].color = sf::Color(color.r, color.g, color.b, color.a);
    target.draw(a);
}

void RenderTarget::drawLine(glm::vec2 start, glm::vec2 end, glm::u8vec4 start_color, glm::u8vec4 end_color)
{
    sf::VertexArray a(sf::LinesStrip, 2);
    a[0].position.x = start.x;
    a[0].position.y = start.y;
    a[1].position.x = end.x;
    a[1].position.y = end.y;
    a[0].color = sf::Color(start_color.r, start_color.g, start_color.b, start_color.a);
    a[1].color = sf::Color(end_color.r, end_color.g, end_color.b, end_color.a);
    target.draw(a);
}

void RenderTarget::drawLine(const std::initializer_list<glm::vec2> points, glm::u8vec4 color)
{
    sf::VertexArray a(sf::LinesStrip, points.size());
    int n=0;
    for(auto point : points)
    {
        a[n].position.x = point.x;
        a[n].position.y = point.y;
        a[n].color = sf::Color(color.r, color.g, color.b, color.a);
        n++;
    }
    target.draw(a);
}

void RenderTarget::drawLine(const std::vector<glm::vec2> points, glm::u8vec4 color)
{
    sf::VertexArray a(sf::LinesStrip, points.size());
    int n=0;
    for(auto point : points)
    {
        a[n].position.x = point.x;
        a[n].position.y = point.y;
        a[n].color = sf::Color(color.r, color.g, color.b, color.a);
        n++;
    }
    target.draw(a);
}

void RenderTarget::drawLineBlendAdd(const std::vector<glm::vec2> points, glm::u8vec4 color)
{
    sf::VertexArray a(sf::LinesStrip, points.size());
    int n=0;
    for(auto point : points)
    {
        a[n].position.x = point.x;
        a[n].position.y = point.y;
        a[n].color = sf::Color(color.r, color.g, color.b, color.a);
        n++;
    }
    target.draw(a, sf::RenderStates(sf::BlendAdd));
}

void RenderTarget::drawPoint(glm::vec2 position, glm::u8vec4 color)
{
    sf::VertexArray a(sf::Points, 1);
    a[0].position.x = position.x;
    a[0].position.y = position.y;
    a[0].color = sf::Color(color.r, color.g, color.b, color.a);
    target.draw(a);
}

void RenderTarget::drawRectColorMultiply(const sp::Rect& rect, glm::u8vec4 color)
{
    sf::RectangleShape overlay(sf::Vector2f(rect.size.x, rect.size.y));
    overlay.setPosition(rect.position.x, rect.position.y);
    overlay.setFillColor(sf::Color(color.r, color.g, color.b, color.a));
    target.draw(overlay, sf::BlendMultiply);
}

void RenderTarget::drawCircleOutline(glm::vec2 center, float radius, float thickness, glm::u8vec4 color)
{
    sf::CircleShape circle(radius - thickness, 50);
    circle.setOrigin(radius - thickness, radius - thickness);
    circle.setPosition(center.x, center.y);
    circle.setFillColor(sf::Color::Transparent);
    circle.setOutlineThickness(thickness);
    circle.setOutlineColor(sf::Color(color.r, color.g, color.b, color.a));
    target.draw(circle);
}

void RenderTarget::drawTiled(const sp::Rect& rect, std::string_view texture)
{
    sf::RectangleShape overlay(sf::Vector2f(rect.size.x, rect.size.y));
    overlay.setPosition(rect.position.x, rect.position.y);
    overlay.setTexture(textureManager.getTexture(texture));
    P<WindowManager> window_manager = engine->getObject("windowManager");
    sf::Vector2i texture_size = window_manager->mapCoordsToPixel(sf::Vector2f(rect.size.x, rect.size.y)) - window_manager->mapCoordsToPixel(sf::Vector2f(0, 0));
    overlay.setTextureRect(sf::IntRect(0, 0, texture_size.x, texture_size.y));
    target.draw(overlay);
}

void RenderTarget::drawTriangleStrip(const std::initializer_list<glm::vec2>& points, glm::u8vec4 color)
{
    sf::VertexArray a(sf::TrianglesStrip, points.size());
    int n=0;
    for(auto point : points)
    {
        a[n].position.x = point.x;
        a[n].position.y = point.y;
        a[n].color = sf::Color(color.r, color.g, color.b, color.a);
        n++;
    }
    target.draw(a);
}

void RenderTarget::drawTriangleStrip(const std::vector<glm::vec2>& points, glm::u8vec4 color)
{
    sf::VertexArray a(sf::TrianglesStrip, points.size());
    int n=0;
    for(auto point : points)
    {
        a[n].position.x = point.x;
        a[n].position.y = point.y;
        a[n].color = sf::Color(color.r, color.g, color.b, color.a);
        n++;
    }
    target.draw(a);
}

void RenderTarget::fillCircle(glm::vec2 center, float radius, glm::u8vec4 color)
{
    sf::CircleShape circle(radius, 50);
    circle.setOrigin(radius, radius);
    circle.setPosition(center.x, center.y);
    circle.setFillColor(sf::Color(color.r, color.g, color.b, color.a));
    target.draw(circle);
}

void RenderTarget::fillRect(const sp::Rect& rect, glm::u8vec4 color)
{
    sf::RectangleShape shape(sf::Vector2f(rect.size.x, rect.size.y));
    shape.setPosition(rect.position.x, rect.position.y);
    shape.setFillColor(sf::Color(color.r, color.g, color.b, color.a));
    target.draw(shape);
}


void RenderTarget::drawTexturedQuad(std::string_view texture,
    glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, glm::vec2 p3,
    glm::vec2 uv0, glm::vec2 uv1, glm::vec2 uv2, glm::vec2 uv3,
    glm::u8vec4 color)
{
    auto tex = textureManager.getTexture(texture);

    sf::VertexArray a(sf::TrianglesFan, 4);
    a[0].position = sf::Vector2f(p0.x, p0.y);
    a[1].position = sf::Vector2f(p1.x, p1.y);
    a[2].position = sf::Vector2f(p2.x, p2.y);
    a[3].position = sf::Vector2f(p3.x, p3.y);
    a[0].texCoords = sf::Vector2f(uv0.x * tex->getSize().x, uv0.y * tex->getSize().y);
    a[1].texCoords = sf::Vector2f(uv1.x * tex->getSize().x, uv1.y * tex->getSize().y);
    a[2].texCoords = sf::Vector2f(uv2.x * tex->getSize().x, uv2.y * tex->getSize().y);
    a[3].texCoords = sf::Vector2f(uv3.x * tex->getSize().x, uv3.y * tex->getSize().y);
    a[0].color = sf::Color(color.r, color.g, color.b, color.a);
    a[1].color = sf::Color(color.r, color.g, color.b, color.a);
    a[2].color = sf::Color(color.r, color.g, color.b, color.a);
    a[3].color = sf::Color(color.r, color.g, color.b, color.a);
    target.draw(a, tex);
}

void RenderTarget::drawText(sp::Rect rect, std::string_view text, Alignment align, float font_size, sf::Font* font, glm::u8vec4 color)
{
    if (!font)
        font = default_font;
    sf::Text textElement(sf::String::fromUtf8(std::begin(text), std::end(text)), *font, font_size);
    float y = 0;
    float x = 0;

    //The "base line" of the text draw is the "Y position where the text is drawn" + font_size.
    //The height of normal text is 70% of the font_size.
    //So use those properties to align the text. Depending on the localbounds does not work.
    switch(align)
    {
    case Alignment::TopLeft:
    case Alignment::TopRight:
    case Alignment::TopCenter:
        y = rect.position.y - 0.3 * font_size;
        break;
    case Alignment::BottomLeft:
    case Alignment::BottomRight:
    case Alignment::BottomCenter:
        y = rect.position.y + rect.size.y - font_size;
        break;
    case Alignment::CenterLeft:
    case Alignment::CenterRight:
    case Alignment::Center:
        y = rect.position.y + rect.size.y / 2.0 - font_size + font_size * 0.35;
        break;
    }

    switch(align)
    {
    case Alignment::TopLeft:
    case Alignment::BottomLeft:
    case Alignment::CenterLeft:
        x = rect.position.x - textElement.getLocalBounds().left;
        break;
    case Alignment::TopRight:
    case Alignment::BottomRight:
    case Alignment::CenterRight:
        x = rect.position.x + rect.size.x - textElement.getLocalBounds().width - textElement.getLocalBounds().left;
        break;
    case Alignment::TopCenter:
    case Alignment::BottomCenter:
    case Alignment::Center:
        x = rect.position.x + rect.size.x / 2.0 - textElement.getLocalBounds().width / 2.0 - textElement.getLocalBounds().left;
        break;
    }
    textElement.setPosition(x, y);
    textElement.setColor(sf::Color(color.r, color.g, color.b, color.a));
    target.draw(textElement);
}

void RenderTarget::drawVerticalText(sp::Rect rect, std::string_view text, Alignment align, float font_size, sf::Font* font, glm::u8vec4 color)
{
    if (!font)
        font = default_font;

    sf::Text textElement(sf::String::fromUtf8(std::begin(text), std::end(text)), *font, font_size);
    textElement.setRotation(-90);
    float x = 0;
    float y = 0;
    x = rect.position.x + rect.size.x / 2.0 - textElement.getLocalBounds().height / 2.0 - textElement.getLocalBounds().top;
    switch(align)
    {
    case Alignment::TopLeft:
    case Alignment::BottomLeft:
    case Alignment::CenterLeft:
        y = rect.position.y + rect.size.y;
        break;
    case Alignment::TopRight:
    case Alignment::BottomRight:
    case Alignment::CenterRight:
        y = rect.position.y + textElement.getLocalBounds().left + textElement.getLocalBounds().width;
        break;
    case Alignment::TopCenter:
    case Alignment::BottomCenter:
    case Alignment::Center:
        y = rect.position.y + rect.size.y / 2.0 + textElement.getLocalBounds().width / 2.0 + textElement.getLocalBounds().left;
        break;
    }
    textElement.setPosition(x, y);
    textElement.setColor(sf::Color(color.r, color.g, color.b, color.a));
    target.draw(textElement);
}

void RenderTarget::draw9Cut(sp::Rect rect, std::string_view texture, glm::u8vec4 color, float width_factor)
{
    sf::Sprite sprite;
    textureManager.setTexture(sprite, texture);
    sf::IntRect textureSize = sprite.getTextureRect();
    int cornerSizeT = textureSize.height / 3;
    float cornerSizeR = cornerSizeT;
    float scale = 1.0;
    if (cornerSizeT > rect.size.y / 2)
    {
        scale = float(rect.size.y / 2) / cornerSizeR;
        sprite.setScale(scale, scale);
        cornerSizeR *= scale;
    }else if (cornerSizeT > rect.size.x / 2)
    {
        scale = float(rect.size.x / 2) / cornerSizeR;
        sprite.setScale(scale, scale);
        cornerSizeR *= scale;
    }

    sprite.setColor(sf::Color(color.r, color.g, color.b, color.a));
    sprite.setOrigin(0, 0);

    float w = 1.0;
    if (cornerSizeR > rect.size.x * width_factor)
        w = rect.size.x * width_factor / cornerSizeR;

    //TopLeft
    sprite.setPosition(rect.position.x, rect.position.y);
    sprite.setTextureRect(sf::IntRect(0, 0, cornerSizeT * w, cornerSizeT));
    target.draw(sprite);
    //BottomLeft
    sprite.setPosition(rect.position.x, rect.position.y + rect.size.y - cornerSizeR);
    sprite.setTextureRect(sf::IntRect(0, textureSize.height - cornerSizeT, cornerSizeT * w, cornerSizeT));
    target.draw(sprite);

    if (rect.size.y > cornerSizeR * 2)
    {
        //left
        sprite.setPosition(rect.position.x, rect.position.y + cornerSizeR);
        sprite.setTextureRect(sf::IntRect(0, cornerSizeT, cornerSizeT * w, 1));
        sprite.setScale(scale, rect.size.y - cornerSizeR*2);
        target.draw(sprite);
        sprite.setScale(scale, scale);
    }
    if (w < 1.0)
        return;

    if (rect.size.x - cornerSizeR > rect.size.x * width_factor)
        w = (width_factor - cornerSizeR / rect.size.x) * (rect.size.x / (rect.size.x - cornerSizeR * 2));

    if (rect.size.x > cornerSizeR * 2)
    {
        //Top
        sprite.setPosition(rect.position.x + cornerSizeR, rect.position.y);
        sprite.setTextureRect(sf::IntRect(cornerSizeT, 0, textureSize.width - cornerSizeT * 2, cornerSizeT));
        sprite.setScale((rect.size.x - cornerSizeR*2) / float(textureSize.width - cornerSizeT * 2) * w, scale);
        target.draw(sprite);
        //Bottom
        sprite.setPosition(rect.position.x + cornerSizeR, rect.position.y + rect.size.y - cornerSizeR);
        sprite.setTextureRect(sf::IntRect(cornerSizeT, textureSize.height - cornerSizeT, textureSize.width - cornerSizeT * 2, cornerSizeT));
        sprite.setScale((rect.size.x - cornerSizeR*2) / float(textureSize.width - cornerSizeT * 2) * w, scale);
        target.draw(sprite);
        sprite.setScale(scale, scale);
    }

    if (rect.size.x > cornerSizeR * 2 && rect.size.y > cornerSizeR * 2)
    {
        //Center
        sprite.setPosition(rect.position.x + cornerSizeR, rect.position.y + cornerSizeR);
        sprite.setTextureRect(sf::IntRect(cornerSizeT, cornerSizeT, 1, 1));
        sprite.setScale((rect.size.x - cornerSizeR*2) * w, rect.size.y - cornerSizeR*2);
        target.draw(sprite);
        sprite.setScale(scale, scale);
    }
    if (w < 1.0)
        return;
    if (width_factor < 1.0)
        w = (width_factor - (rect.size.x - cornerSizeR) / rect.size.x) * (rect.size.x / cornerSizeR);

    //TopRight
    sprite.setPosition(rect.position.x + rect.size.x - cornerSizeR, rect.position.y);
    sprite.setTextureRect(sf::IntRect(textureSize.width - cornerSizeT, 0, cornerSizeT * w, cornerSizeT));
    target.draw(sprite);
    //BottomRight
    sprite.setPosition(rect.position.x + rect.size.x - cornerSizeR, rect.position.y + rect.size.y - cornerSizeR);
    sprite.setTextureRect(sf::IntRect(textureSize.width - cornerSizeT, textureSize.height - cornerSizeT, cornerSizeT * w, cornerSizeT));
    target.draw(sprite);

    if (rect.size.y > cornerSizeR * 2)
    {
        //Right
        sprite.setPosition(rect.position.x + rect.size.x - cornerSizeR, rect.position.y + cornerSizeR);
        sprite.setTextureRect(sf::IntRect(textureSize.width - cornerSizeT, cornerSizeT, cornerSizeT * w, 1));
        sprite.setScale(scale, rect.size.y - cornerSizeR*2);
        target.draw(sprite);
    }
}

void RenderTarget::draw9CutV(sp::Rect rect, std::string_view texture, glm::u8vec4 color, float height_factor)
{
    sf::Sprite sprite;
    textureManager.setTexture(sprite, texture);
    sf::IntRect textureSize = sprite.getTextureRect();
    int cornerSizeT = textureSize.height / 3;
    float cornerSizeR = cornerSizeT;
    float scale = 1.0;
    if (cornerSizeT > rect.size.y / 2)
    {
        scale = float(rect.size.y / 2) / cornerSizeR;
        sprite.setScale(scale, scale);
        cornerSizeR *= scale;
    }else if (cornerSizeT > rect.size.x / 2)
    {
        scale = float(rect.size.x / 2) / cornerSizeR;
        sprite.setScale(scale, scale);
        cornerSizeR *= scale;
    }

    sprite.setColor(sf::Color(color.r, color.g, color.b, color.a));
    sprite.setOrigin(0, 0);

    float h = 1.0;
    if (cornerSizeR > rect.size.y * height_factor)
        h = rect.size.y * height_factor / cornerSizeR;

    //BottomLeft
    sprite.setPosition(rect.position.x, rect.position.y + rect.size.y - cornerSizeR * h);
    sprite.setTextureRect(sf::IntRect(0, textureSize.height - cornerSizeT * h, cornerSizeT, cornerSizeT * h));
    target.draw(sprite);
    //BottomRight
    sprite.setPosition(rect.position.x + rect.size.x - cornerSizeR, rect.position.y + rect.size.y - cornerSizeR * h);
    sprite.setTextureRect(sf::IntRect(textureSize.width - cornerSizeT, textureSize.height - cornerSizeT * h, cornerSizeT, cornerSizeT * h));
    target.draw(sprite);

    if (rect.size.x > cornerSizeR * 2)
    {
        //Bottom
        sprite.setPosition(rect.position.x + cornerSizeR, rect.position.y + rect.size.y - cornerSizeR * h);
        sprite.setTextureRect(sf::IntRect(cornerSizeT, textureSize.height - cornerSizeT * h, textureSize.width - cornerSizeT * 2, cornerSizeT * h));
        sprite.setScale((rect.size.x - cornerSizeR*2) / float(textureSize.width - cornerSizeT * 2), scale);
        target.draw(sprite);
        sprite.setScale(scale, scale);
    }

    if (h < 1.0)
        return;

    if (rect.size.y - cornerSizeR > rect.size.y * height_factor)
        h = (height_factor - cornerSizeR / rect.size.y) * (rect.size.y / (rect.size.y - cornerSizeR * 2));

    if (rect.size.y > cornerSizeR * 2)
    {
        //left
        sprite.setPosition(rect.position.x, rect.position.y + cornerSizeR + (rect.size.y - cornerSizeR * 2) * (1.0f - h));
        sprite.setTextureRect(sf::IntRect(0, cornerSizeT, cornerSizeT, 1));
        sprite.setScale(scale, (rect.size.y - cornerSizeR*2) * h);
        target.draw(sprite);
        sprite.setScale(scale, scale);
        //Right
        sprite.setPosition(rect.position.x + rect.size.x - cornerSizeR, rect.position.y + cornerSizeR + (rect.size.y - cornerSizeR * 2) * (1.0f - h));
        sprite.setTextureRect(sf::IntRect(textureSize.width - cornerSizeT, cornerSizeT, cornerSizeT, 1));
        sprite.setScale(scale, (rect.size.y - cornerSizeR*2) * h);
        target.draw(sprite);
    }

    if (rect.size.x > cornerSizeR * 2 && rect.size.y > cornerSizeR * 2)
    {
        //Center
        sprite.setPosition(rect.position.x + cornerSizeR, rect.position.y + cornerSizeR + (rect.size.y - cornerSizeR * 2) * (1.0f - h));
        sprite.setTextureRect(sf::IntRect(cornerSizeT, cornerSizeT, 1, 1));
        sprite.setScale(rect.size.x - cornerSizeR*2, (rect.size.y - cornerSizeR*2) * h);
        target.draw(sprite);
        sprite.setScale(scale, scale);
    }

    if (h < 1.0)
        return;
    if (height_factor < 1.0)
        h = (height_factor - (rect.size.y - cornerSizeR) / rect.size.y) * (rect.size.y / cornerSizeR);

    //TopLeft
    sprite.setPosition(rect.position.x, rect.position.y + cornerSizeR * (1.0 - h));
    sprite.setTextureRect(sf::IntRect(0, cornerSizeT * (1.0 - h), cornerSizeT, cornerSizeT * h));
    target.draw(sprite);
    //TopRight
    sprite.setPosition(rect.position.x + rect.size.x - cornerSizeR, rect.position.y + cornerSizeR * (1.0 - h));
    sprite.setTextureRect(sf::IntRect(textureSize.width - cornerSizeT, cornerSizeT * (1.0 - h), cornerSizeT, cornerSizeT * h));
    target.draw(sprite);

    if (rect.size.y > cornerSizeR * 2)
    {
        //Top
        sprite.setPosition(rect.position.x + cornerSizeR, rect.position.y + cornerSizeR * (1.0 - h));
        sprite.setTextureRect(sf::IntRect(cornerSizeT, cornerSizeT * (1.0 - h), 1, cornerSizeT * h));
        sprite.setScale(rect.size.x - cornerSizeR*2, scale);
        target.draw(sprite);
    }
}

void RenderTarget::drawStretched(sp::Rect rect, std::string_view texture, glm::u8vec4 color)
{
    if (rect.size.x >= rect.size.y)
    {
        drawStretchedH(rect, texture, color);
    }else{
        drawStretchedV(rect, texture, color);
    }
}

void RenderTarget::drawStretchedH(sp::Rect rect, std::string_view texture, glm::u8vec4 color)
{
    sf::Texture* texture_ptr = textureManager.getTexture(texture);
    sf::Vector2f texture_size = sf::Vector2f(texture_ptr->getSize());
    sf::VertexArray a(sf::TrianglesStrip, 8);

    float w = rect.size.y / 2.0f;
    if (w * 2 > rect.size.x)
        w = rect.size.x / 2.0f;
    a[0].position = sf::Vector2f(rect.position.x, rect.position.y);
    a[1].position = sf::Vector2f(rect.position.x, rect.position.y + rect.size.y);
    a[2].position = sf::Vector2f(rect.position.x + w, rect.position.y);
    a[3].position = sf::Vector2f(rect.position.x + w, rect.position.y + rect.size.y);
    a[4].position = sf::Vector2f(rect.position.x + rect.size.x - w, rect.position.y);
    a[5].position = sf::Vector2f(rect.position.x + rect.size.x - w, rect.position.y + rect.size.y);
    a[6].position = sf::Vector2f(rect.position.x + rect.size.x, rect.position.y);
    a[7].position = sf::Vector2f(rect.position.x + rect.size.x, rect.position.y + rect.size.y);

    a[0].texCoords = sf::Vector2f(0, 0);
    a[1].texCoords = sf::Vector2f(0, texture_size.y);
    a[2].texCoords = sf::Vector2f(texture_size.x / 2, 0);
    a[3].texCoords = sf::Vector2f(texture_size.x / 2, texture_size.y);
    a[4].texCoords = sf::Vector2f(texture_size.x / 2, 0);
    a[5].texCoords = sf::Vector2f(texture_size.x / 2, texture_size.y);
    a[6].texCoords = sf::Vector2f(texture_size.x, 0);
    a[7].texCoords = sf::Vector2f(texture_size.x, texture_size.y);

    for(int n=0; n<8; n++)
        a[n].color = sf::Color(color.r, color.g, color.b, color.a);

    target.draw(a, texture_ptr);
}

void RenderTarget::drawStretchedV(sp::Rect rect, std::string_view texture, glm::u8vec4 color)
{
    sf::Texture* texture_ptr = textureManager.getTexture(texture);
    sf::Vector2f texture_size = sf::Vector2f(texture_ptr->getSize());
    sf::VertexArray a(sf::TrianglesStrip, 8);

    float h = rect.size.x / 2.0;
    if (h * 2 > rect.size.y)
        h = rect.size.y / 2.0f;
    a[0].position = sf::Vector2f(rect.position.x, rect.position.y);
    a[1].position = sf::Vector2f(rect.position.x + rect.size.x, rect.position.y);
    a[2].position = sf::Vector2f(rect.position.x, rect.position.y + h);
    a[3].position = sf::Vector2f(rect.position.x + rect.size.x, rect.position.y + h);
    a[4].position = sf::Vector2f(rect.position.x, rect.position.y + rect.size.y - h);
    a[5].position = sf::Vector2f(rect.position.x + rect.size.x, rect.position.y + rect.size.y - h);
    a[6].position = sf::Vector2f(rect.position.x, rect.position.y + rect.size.y);
    a[7].position = sf::Vector2f(rect.position.x + rect.size.x, rect.position.y + rect.size.y);

    a[0].texCoords = sf::Vector2f(0, 0);
    a[1].texCoords = sf::Vector2f(0, texture_size.y);
    a[2].texCoords = sf::Vector2f(texture_size.x / 2, 0);
    a[3].texCoords = sf::Vector2f(texture_size.x / 2, texture_size.y);
    a[4].texCoords = sf::Vector2f(texture_size.x / 2, 0);
    a[5].texCoords = sf::Vector2f(texture_size.x / 2, texture_size.y);
    a[6].texCoords = sf::Vector2f(texture_size.x, 0);
    a[7].texCoords = sf::Vector2f(texture_size.x, texture_size.y);

    for(int n=0; n<8; n++)
        a[n].color = sf::Color(color.r, color.g, color.b, color.a);

    target.draw(a, texture_ptr);
}

void RenderTarget::drawStretchedHV(sp::Rect rect, float corner_size, std::string_view texture, glm::u8vec4 color)
{
    sf::Texture* texture_ptr = textureManager.getTexture(texture);
    sf::Vector2f texture_size = sf::Vector2f(texture_ptr->getSize());
    sf::VertexArray a(sf::TrianglesStrip, 8);

    for(int n=0; n<8; n++)
        a[n].color = sf::Color(color.r, color.g, color.b, color.a);

    corner_size = std::min(corner_size, rect.size.y / 2.0f);
    corner_size = std::min(corner_size, rect.size.x / 2.0f);

    a[0].position = sf::Vector2f(rect.position.x, rect.position.y);
    a[1].position = sf::Vector2f(rect.position.x, rect.position.y + corner_size);
    a[2].position = sf::Vector2f(rect.position.x + corner_size, rect.position.y);
    a[3].position = sf::Vector2f(rect.position.x + corner_size, rect.position.y + corner_size);
    a[4].position = sf::Vector2f(rect.position.x + rect.size.x - corner_size, rect.position.y);
    a[5].position = sf::Vector2f(rect.position.x + rect.size.x - corner_size, rect.position.y + corner_size);
    a[6].position = sf::Vector2f(rect.position.x + rect.size.x, rect.position.y);
    a[7].position = sf::Vector2f(rect.position.x + rect.size.x, rect.position.y + corner_size);

    a[0].texCoords = sf::Vector2f(0, 0);
    a[1].texCoords = sf::Vector2f(0, texture_size.y / 2.0);
    a[2].texCoords = sf::Vector2f(texture_size.x / 2, 0);
    a[3].texCoords = sf::Vector2f(texture_size.x / 2, texture_size.y / 2.0);
    a[4].texCoords = sf::Vector2f(texture_size.x / 2, 0);
    a[5].texCoords = sf::Vector2f(texture_size.x / 2, texture_size.y / 2.0);
    a[6].texCoords = sf::Vector2f(texture_size.x, 0);
    a[7].texCoords = sf::Vector2f(texture_size.x, texture_size.y / 2.0);

    target.draw(a, texture_ptr);

    a[0].position.y = rect.position.y + rect.size.y - corner_size;
    a[2].position.y = rect.position.y + rect.size.y - corner_size;
    a[4].position.y = rect.position.y + rect.size.y - corner_size;
    a[6].position.y = rect.position.y + rect.size.y - corner_size;

    a[0].texCoords.y = texture_size.y / 2.0;
    a[2].texCoords.y = texture_size.y / 2.0;
    a[4].texCoords.y = texture_size.y / 2.0;
    a[6].texCoords.y = texture_size.y / 2.0;

    target.draw(a, texture_ptr);

    a[1].position.y = rect.position.y + rect.size.y;
    a[3].position.y = rect.position.y + rect.size.y;
    a[5].position.y = rect.position.y + rect.size.y;
    a[7].position.y = rect.position.y + rect.size.y;

    a[1].texCoords.y = texture_size.y;
    a[3].texCoords.y = texture_size.y;
    a[5].texCoords.y = texture_size.y;
    a[7].texCoords.y = texture_size.y;

    target.draw(a, texture_ptr);
}

}

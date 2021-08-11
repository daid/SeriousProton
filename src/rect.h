#ifndef SP_RECT_H
#define SP_RECT_H

#include <cmath>
#include <glm/vec2.hpp>

namespace sp {

class Rect
{
public:
    Rect() : position{0, 0}, size{0, 0} {}
    Rect(glm::vec2 position, glm::vec2 size) : position(position), size(size) {}
    Rect(float x, float y, float w, float h) : position(x, y), size(w, h) {}
    
    glm::vec2 center() const
    {
        return position + size * 0.5f;
    }
    
    bool contains(glm::vec2 p) const
    {
        return p.x >= position.x && p.x <= position.x + size.x && p.y >= position.y && p.y <= position.y + size.y;
    }
    
    bool overlaps(const Rect& other) const
    {
        return position.x + size.x >= other.position.x && other.position.x + other.size.x >= position.x &&
            position.y + size.y >= other.position.y && other.position.y + other.size.y >= position.y;
    }
    
    void growToInclude(glm::vec2 p)
    {
        if (p.x < position.x)
        {
            size.x = size.x - p.x + position.x;
            position.x = p.x;
        }
        if (p.y < position.y)
        {
            size.y = size.y - p.y + position.y;
            position.y = p.y;
        }
        if (p.x > position.x + size.x)
            size.x = p.x - position.x;
        if (p.y > position.y + size.y)
            size.y = p.y - position.y;
    }
    
    void shrinkToFitWithin(const Rect& other)
    {
        if (position.x < other.position.x)
        {
            size.x -= other.position.x - position.x;
            position.x = other.position.x;
        }
        if (position.y < other.position.y)
        {
            size.y -= other.position.y - position.y;
            position.y = other.position.y;
        }
        if (position.x + size.x > other.position.x + other.size.x)
            size.x -= (position.x + size.x) - (other.position.x + other.size.x);
        if (position.y + size.y > other.position.y + other.size.y)
            size.y -= (position.y + size.y) - (other.position.y + other.size.y);
        if (size.x < 0)
        {
            position.x += size.x;
            size.x = 0;
        }
        if (size.y < 0)
        {
            position.y += size.y;
            size.y = 0;
        }
    }
    
    bool operator==(const Rect& other)
    {
        return position == other.position && size == other.size;
    }

    bool operator!=(const Rect& other)
    {
        return position != other.position || size != other.size;
    }

    glm::vec2 position;
    glm::vec2 size;
};

}//namespace sp

#endif//SP_RECT_H

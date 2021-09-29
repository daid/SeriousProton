#include "graphics/textureAtlas.h"
#include "textureManager.h"
#include "GL/glew.h"
#include <algorithm>


namespace sp {

AtlasTexture::AtlasTexture(glm::ivec2 size)
{
    texture_size = size;
    available_areas.push_back({{0, 0}, {size.x, size.y}});
    gl_handle = 0;
    smooth = textureManager.isDefaultSmoothFiltering();
}

AtlasTexture::~AtlasTexture()
{
    if (gl_handle)
        glDeleteTextures(1, &gl_handle);
}

void AtlasTexture::bind()
{
    if (gl_handle == 0)
    {
        glGenTextures(1, &gl_handle);
        glBindTexture(GL_TEXTURE_2D, gl_handle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, smooth ? GL_LINEAR : GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, smooth ? GL_LINEAR : GL_NEAREST);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_size.x, texture_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        //Put 1 white pixel in the {1,1} corner, which we can use to draw solid things.
        glm::u8vec4 white{255,255,255,255};
        glTexSubImage2D(GL_TEXTURE_2D, 0, texture_size.x - 1, texture_size.y - 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &white);
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, gl_handle);
    }

    if (!add_list.empty())
    {
        for(auto& add_item : add_list)
            glTexSubImage2D(GL_TEXTURE_2D, 0, add_item.position.x, add_item.position.y, add_item.image.getSize().x, add_item.image.getSize().y, GL_RGBA, GL_UNSIGNED_BYTE, add_item.image.getPtr());
        
        add_list.clear();
    }
}

bool AtlasTexture::canAdd(const Image& image, int margin)
{
    glm::ivec2 size = image.getSize() + glm::ivec2(margin * 2, margin * 2);
    for(int n=int(available_areas.size()) - 1; n >= 0; n--)
    {
        if (available_areas[n].size.x >= size.x && available_areas[n].size.y >= size.y)
            return true;
    }
    return false;
}

Rect AtlasTexture::add(Image&& image, int margin)
{
    glm::ivec2 size = image.getSize() + glm::ivec2(margin * 2, margin * 2);
    for(int n=int(available_areas.size()) - 1; n >= 0; n--)
    {
        if (available_areas[n].size.x >= size.x && available_areas[n].size.y >= size.y)
        {
            //Suitable area found, grab it, cut it, and put it in a stew.
            RectInt full_area = available_areas[n];
            available_areas.erase(available_areas.begin() + n);
            
            if (full_area.size.x <= full_area.size.y)
            {
                //Split horizontal
                addArea({{full_area.position.x + size.x, full_area.position.y}, {full_area.size.x - size.x, size.y}});
                addArea({{full_area.position.x, full_area.position.y + size.y}, {full_area.size.x, full_area.size.y - size.y}});
            }
            else
            {
                //Split vertical
                addArea({{full_area.position.x + size.x, full_area.position.y}, {full_area.size.x - size.x, full_area.size.y}});
                addArea({{full_area.position.x, full_area.position.y + size.y}, {size.x, full_area.size.y - size.y}});
            }
            
            if (image.getSize().x > 0 && image.getSize().y > 0)
            {
                add_list.emplace_back();
                add_list.back().image = std::move(image);
                add_list.back().position.x = full_area.position.x + margin;
                add_list.back().position.y = full_area.position.y + margin;
            }

            return Rect(
                float(full_area.position.x + margin) / float(texture_size.x), float(full_area.position.y + margin) / float(texture_size.y),
                float(size.x - margin * 2) / float(texture_size.x), float(size.y - margin * 2) / float(texture_size.y));
        }
    }
    return Rect(0, 0, -1, -1);
}

float AtlasTexture::usageRate()
{
    int all_texture_volume = texture_size.x * texture_size.y;
    int unused_texture_volume = 0;
    for(auto& area : available_areas)
        unused_texture_volume += area.size.x * area.size.y;
    return float(all_texture_volume - unused_texture_volume) / float(all_texture_volume);
}

void AtlasTexture::addArea(RectInt area)
{
    //Ignore really slim areas, they are never really used and use up a lot of room in the available_areas otherwise.
    if (area.size.x < 4 || area.size.y < 4)
        return;
    int area_volume = area.size.x * area.size.y;
    auto it = std::lower_bound(available_areas.begin(), available_areas.end(), area_volume, [](const RectInt& r, int volume) -> bool
    {
        return r.size.x * r.size.y > volume;
    });
    available_areas.insert(it, area);
}

}//namespace sp

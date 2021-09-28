#include "graphics/font.h"

namespace sp {

Font::PreparedFontString Font::prepare(std::string_view s, int pixel_size, float text_size, glm::vec2 area_size, Alignment alignment, int flags)
{
    float size_scale = text_size / float(pixel_size);
    float line_spacing = getLineSpacing(pixel_size) * size_scale;
    PreparedFontString result;

    if (flags & FlagVertical)
    {
        std::swap(area_size.x, area_size.y);
        switch(alignment)
        {
        case Alignment::TopLeft:     alignment = Alignment::TopRight; break;
        case Alignment::TopCenter:   alignment = Alignment::CenterRight; break;
        case Alignment::TopRight:    alignment = Alignment::BottomRight; break;
        case Alignment::CenterLeft:  alignment = Alignment::TopCenter; break;
        case Alignment::Center:      alignment = Alignment::Center; break;
        case Alignment::CenterRight: alignment = Alignment::BottomCenter; break;
        case Alignment::BottomLeft:  alignment = Alignment::TopLeft; break;
        case Alignment::BottomCenter:alignment = Alignment::CenterLeft; break;
        case Alignment::BottomRight: alignment = Alignment::BottomLeft; break;
        }
    }

    result.font = this;
    result.alignment = alignment;
    result.text_size = text_size;
    result.pixel_size = pixel_size;
    result.area_size = area_size;
    result.flags = flags;

    glm::vec2 position(0, 0);
    int previous_char_code = -1;

    for(unsigned int index=0; index<s.size(); )
    {
        CharacterInfo char_info = getCharacterInfo(&s[index]);
        if (previous_char_code > -1)
        {
            position.x += getKerning(previous_char_code, char_info.code) * size_scale;
        }
        result.data.push_back({
            .position = position,
            .char_code = char_info.code,
            .string_offset = int(index),
        });
        previous_char_code = char_info.code;
        index += char_info.consumed_bytes;

        if (char_info.code == '\n')
        {
            result.data.back().char_code = 0;
            position.x = 0;
            position.y += line_spacing;
            continue;
        }

        GlyphInfo glyph;
        if (!getGlyphInfo(char_info.code, pixel_size, glyph))
        {
            glyph.advance = 0;
            glyph.bounds.size.x = 0;
        }

        position.x += glyph.advance * size_scale;
        if ((flags & FlagLineWrap) && position.x > area_size.x)
        {
            //Try to wrap the line by going back to the last space character and replace that with a newline.
            for(int n=result.data.size()-2; (n > 0) && (result.data[n].char_code != 0); n--)
            {
                if (result.data[n].char_code == ' ')
                {
                    result.data[n].char_code = 0;
                    index = result.data[n + 1].string_offset;
                    result.data.resize(n + 1);
                    position.x = 0.0f;
                    position.y += line_spacing;
                    break;
                }
            }

            if (result.lastLineCharacterCount() > 1)
            {
                // If line wrapping by space-replacement failed. Chop off characters till we are inside the area.
                while(result.lastLineCharacterCount() > 2 && result.data.back().position.x > area_size.x)
                {
                    result.data.pop_back();
                }
                index = result.data.back().string_offset;
                result.data.back().char_code = 0;
                result.data.back().string_offset = -1;
                position.x = 0.0f;
                position.y += line_spacing;
            }
        }
    }
    result.data.push_back({
        .position = position,
        .char_code = 0,
        .string_offset = int(s.size()),
    });

    result.alignAll();

    return result;
}

void Font::PreparedFontString::alignAll()
{
    float size_scale = text_size / float(pixel_size);
    float line_spacing = font->getLineSpacing(pixel_size) * size_scale;

    auto start_of_line = data.begin();
    for(auto it = data.begin(); it != data.end(); ++it)
    {
        if (it->char_code == 0)
        {
            float offset = 0.0f;
            switch(alignment)
            {
            case Alignment::TopLeft:
            case Alignment::BottomLeft:
            case Alignment::CenterLeft:
                offset = 0.0f;
                break;
            case Alignment::TopCenter:
            case Alignment::Center:
            case Alignment::BottomCenter:
                offset = (area_size.x - it->position.x) * 0.5f;
                break;
            case Alignment::TopRight:
            case Alignment::CenterRight:
            case Alignment::BottomRight:
                offset = area_size.x - it->position.x;
                break;
            }
            while(start_of_line != it)
            {
                start_of_line->position.x += offset;
                ++start_of_line;
            }
            start_of_line->position.x += offset;
            ++start_of_line;
        }
    }

    float offset = 0.0;
    switch(alignment)
    {
    case Alignment::TopLeft:
    case Alignment::TopCenter:
    case Alignment::TopRight:
        offset = line_spacing;
        break;
    case Alignment::CenterLeft:
    case Alignment::Center:
    case Alignment::CenterRight:
        offset = (area_size.y - line_spacing * (getLineCount() - 1)) * 0.5f;
        offset += font->getBaseline(pixel_size) * size_scale * 0.5f;
        break;
    case Alignment::BottomLeft:
    case Alignment::BottomCenter:
    case Alignment::BottomRight:
        offset = area_size.y;
        break;
    }
    for(GlyphData& d : data)
    {
        d.position.y += offset;
    }
}

glm::vec2 Font::PreparedFontString::getUsedAreaSize() const
{
    glm::vec2 result(getMaxLineWidth(), (float(getLineCount()) + 0.3) * font->getLineSpacing(pixel_size) * text_size / float(pixel_size));
    if (flags & FlagVertical)
        std::swap(result.x, result.y);
    return result;
}
/*
std::shared_ptr<MeshData> Font::PreparedFontString::create()
{
    float size_scale = text_size / float(pixel_size);

    MeshData::Vertices vertices;
    MeshData::Indices indices;
    
    vertices.reserve(data.size() * 6);
    indices.reserve(data.size() * 4);

    for(const GlyphData& d : data)
    {
        GlyphInfo glyph;
        if (d.char_code == 0 || !font->getGlyphInfo(d.char_code, pixel_size, glyph))
        {
            glyph.advance = 0.0f;
            glyph.bounds.size.x = 0.0f;
        }

        if (glyph.bounds.size.x > 0.0f)
        {
            float u0 = glyph.uv_rect.position.x;
            float v0 = glyph.uv_rect.position.y;
            float u1 = glyph.uv_rect.position.x + glyph.uv_rect.size.x;
            float v1 = glyph.uv_rect.position.y + glyph.uv_rect.size.y;
            
            float left = d.position.x + glyph.bounds.position.x * size_scale;
            float right = left + glyph.bounds.size.x * size_scale;
            float top = d.position.y + glyph.bounds.position.y * size_scale;
            float bottom = top - glyph.bounds.size.y * size_scale;
            
            if (flags & FlagClip)
            {
                if (right < 0)
                    continue;
                if (left < 0)
                {
                    u0 = u1 - glyph.uv_rect.size.x * (0 - right) / (left - right);
                    left = 0;
                }

                if (left > area_size.x)
                    continue;
                if (right > area_size.x)
                {
                    u1 = u0 + glyph.uv_rect.size.x * (area_size.x - left) / (right - left);
                    right = area_size.x;
                }

                if (top < 0)
                    continue;
                if (bottom < 0)
                {
                    v1 = v0 + glyph.uv_rect.size.y * (0 - top) / (bottom - top);
                    bottom = 0;
                }

                if (bottom > area_size.y)
                    continue;
                if (top > area_size.y)
                {
                    v0 = v1 - glyph.uv_rect.size.y * (area_size.y - bottom) / (top - bottom);
                    top = area_size.y;
                }
            }

            Vector3f p0(left, top, 0.0f);
            Vector3f p1(right, top, 0.0f);
            Vector3f p2(left, bottom, 0.0f);
            Vector3f p3(right, bottom, 0.0f);

            indices.emplace_back(vertices.size() + 0);
            indices.emplace_back(vertices.size() + 2);
            indices.emplace_back(vertices.size() + 1);
            indices.emplace_back(vertices.size() + 2);
            indices.emplace_back(vertices.size() + 3);
            indices.emplace_back(vertices.size() + 1);

            if (flags & FlagVertical)
            {
                p0 = Vector3f(area_size.y - p0.y, p0.x, 0.0f);
                p1 = Vector3f(area_size.y - p1.y, p1.x, 0.0f);
                p2 = Vector3f(area_size.y - p2.y, p2.x, 0.0f);
                p3 = Vector3f(area_size.y - p3.y, p3.x, 0.0f);
            }

            vertices.emplace_back(p0, d.normal, Vector2f(u0, v0));
            vertices.emplace_back(p1, d.normal, Vector2f(u1, v0));
            vertices.emplace_back(p2, d.normal, Vector2f(u0, v1));
            vertices.emplace_back(p3, d.normal, Vector2f(u1, v1));
        }
    }

    return std::make_shared<MeshData>(std::move(vertices), std::move(indices));
}
*/
float Font::PreparedFontString::getMaxLineWidth() const
{
    float max_x = 0.0f;
    for(auto& d : data)
        max_x = std::max(max_x, d.position.x);
    return max_x;
}

int Font::PreparedFontString::getLineCount() const
{
    int count = 0;
    for(auto& d : data)
        if (d.char_code == 0)
            count += 1;
    return count;
}

int Font::PreparedFontString::lastLineCharacterCount() const
{
    for(int n=data.size()-1; n >= 0; n--)
    {
        if (data[n].char_code == 0)
            return data.size() - n - 1;
    }
    return data.size();
}

}//namespace sp

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
            /*.position =*/ position,
            /*.char_code =*/ char_info.code,
            /*.string_offset =*/ int(index),
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
            //Try to wrap the line by going back to the last space character and replace that with a newline. If a '-' is found first, keep it and just add a newline.
            for(int n=static_cast<int>(result.data.size())-2; (n > 0) && (result.data[n].char_code != 0); n--)
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
                if (result.data[n].char_code == '-')
                {
                    index = result.data[n + 1].string_offset;
                    result.data.resize(n + 1);
                    result.data.push_back(result.data.back());
                    result.data.back().char_code = 0;
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
        /*.position =*/ position,
        /*.char_code =*/ 0,
        /*.string_offset =*/ int(s.size()),
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

    float offset = 0.0f;
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
    glm::vec2 result(getMaxLineWidth(), (float(getLineCount()) + 0.3f) * font->getLineSpacing(pixel_size) * text_size / float(pixel_size));
    if (flags & FlagVertical)
        std::swap(result.x, result.y);
    return result;
}

Font* Font::PreparedFontString::getFont() const
{
    return font;
}

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

size_t Font::PreparedFontString::lastLineCharacterCount() const
{
    for(int n=static_cast<int>(data.size())-1; n >= 0; n--)
    {
        if (data[n].char_code == 0)
            return data.size() - n - 1;
    }
    return data.size();
}

}//namespace sp

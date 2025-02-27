#include "graphics/font.h"

namespace sp {

Font::PreparedFontString Font::start(int pixel_size, glm::vec2 area_size, Alignment alignment, int flags)
{
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
    result.pixel_size = pixel_size;
    result.area_size = area_size;
    result.flags = flags;
    return result;
}

Font::PreparedFontString Font::prepare(std::string_view s, int pixel_size, float text_size, glm::u8vec4 color, glm::vec2 area_size, Alignment alignment, int flags)
{
    auto result = start(pixel_size, area_size, alignment, flags);
    result.append(s, text_size, color);
    result.finish();
    return result;
}

void Font::PreparedFontString::append(std::string_view s, float text_size, glm::u8vec4 color)
{
    int previous_char_code = -1;
    int string_offset = 0;
    if (!data.empty()) {
        previous_char_code = data.back().char_code;
        string_offset = data.back().string_offset + 1;
    }
    for(unsigned int index=0; index<s.size(); )
    {
        CharacterInfo char_info = font->getCharacterInfo(&s[index]);
        if (previous_char_code > -1)
            next_position_x += font->getKerning(previous_char_code, char_info.code) * text_size / float(pixel_size);
        data.push_back({
            /*.position =*/ {next_position_x, 0.0f},
            /*.char_code =*/ char_info.code,
            /*.string_offset =*/ int(string_offset + index),
            /*.size =*/ text_size,
            /*.color =*/ color,
        });
        previous_char_code = char_info.code;
        index += char_info.consumed_bytes;

        if (char_info.code == '\n')
        {
            data.back().char_code = 0;
            next_position_x = 0.0f;
            continue;
        }

        GlyphInfo glyph;
        if (!font->getGlyphInfo(char_info.code, pixel_size, glyph))
        {
            glyph.advance = 0;
            glyph.bounds.size.x = 0;
        }

        next_position_x += glyph.advance * text_size / float(pixel_size);
        if ((flags & FlagLineWrap) && next_position_x > area_size.x)
        {
            //Try to wrap the line by going back to the last space character and replace that with a newline. If a '-' is found first, keep it and just add a newline.
            for(int n=static_cast<int>(data.size())-2; (n > 0) && (data[n].char_code != 0); n--)
            {
                if (data[n].char_code == ' ')
                {
                    data[n].char_code = 0;
                    index = data[n + 1].string_offset;
                    data.resize(n + 1);
                    next_position_x = 0.0f;
                    break;
                }
                if (data[n].char_code == '-')
                {
                    index = data[n + 1].string_offset - string_offset;
                    data.resize(n + 1);
                    data.push_back(data.back());
                    data.back().char_code = 0;
                    next_position_x = 0.0f;
                    break;
                }
            }

            if (lastLineCharacterCount() > 1)
            {
                // If line wrapping by space-replacement failed. Chop off characters till we are inside the area.
                while(lastLineCharacterCount() > 2 && data.back().position.x > area_size.x)
                {
                    data.pop_back();
                }
                index = data.back().string_offset - string_offset;
                data.back().char_code = 0;
                data.back().string_offset = -1;
                next_position_x = 0.0f;
            }
        }
    }
}

void Font::PreparedFontString::finish()
{
    if (data.empty()) {
        data.push_back({
            /*.position =*/ {next_position_x, 0.0f},
            /*.char_code =*/ 0,
            /*.string_offset =*/ 0,
            /*.size =*/ 0.0f,
            /*.color =*/ {255,255,255,255},
        });
    } else {
        data.push_back({
            /*.position =*/ {next_position_x, 0.0f},
            /*.char_code =*/ 0,
            /*.string_offset =*/ data.back().string_offset + 1,
            /*.size =*/ data.back().size,
            /*.color =*/ data.back().color,
        });
    }

    float current_line_spacing = 0.0f;
    float total_line_spacing = 0.0f;
    float last_line_spacing = 0.0f;
    float max_y = 0.0f;
    auto start_of_line = data.begin();
    for(auto it = data.begin(); it != data.end(); ++it)
    {
        current_line_spacing = std::max(current_line_spacing, font->getLineSpacing(pixel_size) * it->size / float(pixel_size));
        if (it->char_code == 0)
        {
            total_line_spacing += current_line_spacing;
            for(auto i = start_of_line; i != it; i++) {
                i->position.y += total_line_spacing;
                max_y = std::max(max_y, i->position.y);
            }
            it->position.y += total_line_spacing;
            last_line_spacing = current_line_spacing;
            current_line_spacing = 0.0f;
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
        offset = 0.0f;
        break;
    case Alignment::CenterLeft:
    case Alignment::Center:
    case Alignment::CenterRight:
        offset = (area_size.y - max_y) * 0.5f - last_line_spacing * 0.15f;
        break;
    case Alignment::BottomLeft:
    case Alignment::BottomCenter:
    case Alignment::BottomRight:
        offset = area_size.y - last_line_spacing * 0.3f;
        break;
    }
    for(GlyphData& d : data)
    {
        d.position.y += offset;
    }
}

glm::vec2 Font::PreparedFontString::getUsedAreaSize() const
{
    float max_x = 0.0f;
    float max_y = 0.0f;
    for(auto& d : data) {
        max_x = std::max(max_x, d.position.x);
        max_y = std::max(max_y, (float(getLineCount()) + 0.3f) * d.size);
    }

    glm::vec2 result(max_x, max_y);
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

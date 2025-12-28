#ifndef SP_GRAPHICS_FONT_H
#define SP_GRAPHICS_FONT_H

#include "nonCopyable.h"
#include "rect.h"
#include "graphics/alignment.h"
#include "graphics/image.h"
#include <vector>
#include <string_view>


namespace sp {

class Font : sp::NonCopyable
{
public:
    static constexpr int FlagLineWrap = 0x01;
    static constexpr int FlagClip = 0x02;
    static constexpr int FlagVertical = 0x04;

    class PreparedFontString
    {
    public:
        class GlyphData
        {
        public:
            glm::vec2 position;
            int char_code;
            int string_offset;
            float size;
            glm::u8vec4 color;
        };
        std::vector<GlyphData> data;

        glm::vec2 getUsedAreaSize() const;
        Font* getFont() const;

        void append(std::string_view s, float text_size, glm::u8vec4 color);
        void finish();
    private:
        float next_position_x = 0.0f;
        Font* font = nullptr;
        Alignment alignment;
        int pixel_size;
        glm::vec2 area_size;
        int flags;

        float getMaxLineWidth() const;
        int getLineCount() const;
        size_t lastLineCharacterCount() const;

        friend class Font;
    };
    PreparedFontString start(int pixel_size, glm::vec2 area_size, Alignment alignment, int flags=0);
    PreparedFontString prepare(std::string_view s, int pixel_size, float text_size, glm::u8vec4 color, glm::vec2 area_size, Alignment alignment, int flags=0);

    class CharacterInfo
    {
    public:
        int code;
        int consumed_bytes;
    };
    class GlyphInfo
    {
    public:
        Rect bounds;
        float advance;
    };
    virtual CharacterInfo getCharacterInfo(const char* str) = 0;
    virtual bool getGlyphInfo(int char_code, int pixel_size, GlyphInfo& info) = 0;
    virtual Image drawGlyph(int char_code, int pixel_size) = 0;
    virtual float getLineSpacing(int pixel_size) = 0;
    virtual float getBaseline(int pixel_size) = 0;
    virtual float getKerning(int previous_char_code, int current_char_code) = 0;

    virtual void setBaselineOffset(float offset) { baseline_offset = offset; }
    virtual float getBaselineOffset() const { return baseline_offset; }

private:
    float baseline_offset = 0.0f;
};

}

#endif//SP_GRAPHICS_FONT_H

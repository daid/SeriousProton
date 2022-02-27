#ifndef SP_GRAPHICS_FREETYPEFONT_H
#define SP_GRAPHICS_FREETYPEFONT_H

#include "graphics/font.h"
#include "resources.h"
#include <unordered_map>


namespace sp {

class FreetypeFont : public Font
{
public:
    FreetypeFont(const string& name, P<ResourceStream> stream);
    ~FreetypeFont();

protected:
    virtual CharacterInfo getCharacterInfo(const char* str) override;
    virtual bool getGlyphInfo(int char_code, int pixel_size, GlyphInfo& info) override;
    virtual Image drawGlyph(int char_code, int pixel_size) override;
    virtual float getLineSpacing(int pixel_size) override;
    virtual float getBaseline(int pixel_size) override;
    virtual float getKerning(int previous_char_code, int current_char_code) override;

private:
    void* ft_library;
    void* ft_face;
    void* ft_stream_rec;
    
    //We need to keep the resource stream open, as the freetype keeps it open as well.
    //So we store the reference here.
    P<ResourceStream> font_resource_stream;
    
    //Keep track of glyphs that are loaded in the texture already.
    //As soon as we load a new glyph, the texture becomes invalid and needs to be updated.
    std::unordered_map<int, std::unordered_map<int, GlyphInfo>> loaded_glyphs;
};

}

#endif//SP_GRAPHICS_FREETYPEFONT_H

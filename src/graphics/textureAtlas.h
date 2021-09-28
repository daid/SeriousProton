#ifndef SP_GRAPHICS_TEXTURE_ATLAS_H
#define SP_GRAPHICS_TEXTURE_ATLAS_H

#include "graphics/texture.h"
#include "rect.h"


namespace sp {

/**
    An AtlasTexture is a texture that contains multiple images layed out inside the same texture unit.
    The advantage of this is that there are less texture state changes during rendering, which is an inefficient
    action.
 */
class AtlasTexture : public Texture
{
public:
    AtlasTexture(glm::ivec2 size);
    virtual ~AtlasTexture();
    
    virtual void bind() override;

    //Only check if we can add this image, while this does the same work as add(), it does not claim ownership of the image
    //And thus the image can be placed somewhere else if this check fails.
    bool canAdd(const Image& image, int margin=0);
    
    //Add an image to the atlas and return the area where the image is located in normalized coordinates.
    //Returns a negative size if the image cannot be added.
    Rect add(Image&& image, int margin=0);
    
    //Return between 0.0 and 1.0 to indicate how much area of this texture is already used.
    // Where 0.0 is fully empty and 1.0 is fully used (never really happens due to overhead)
    float usageRate();
private:
    struct RectInt
    {
        glm::ivec2 position;
        glm::ivec2 size;
    };
    void addArea(RectInt area);

    bool smooth;
    unsigned int gl_handle;

    glm::ivec2 texture_size;
    std::vector<RectInt> available_areas;
    
    class ToAdd
    {
    public:
        Image image;
        glm::ivec2 position;
    };
    std::vector<ToAdd> add_list;
};

}//namespace sp

#endif//SP_GRAPHICS_TEXTURE_ATLAS_H

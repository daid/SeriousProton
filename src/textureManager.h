#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <unordered_map>
#include <vector>
#include "stringImproved.h"
#include "graphics/texture.h"

class TextureManager;
extern TextureManager textureManager;
class TextureManager
{
private:
    uint32_t baseMipLevel = 0;
    bool defaultRepeated;
    bool defaultSmooth;
    bool autoSprite;
    bool disabled;  //Allow to disable to texture manager, which does not load anything. For headless runs.
    std::unordered_map<string, sp::Texture*> textureMap;
public:
    TextureManager();
    ~TextureManager();

    void setBaseMipLevel(uint32_t baseMip) { baseMipLevel = baseMip; }
    void setDefaultRepeated(bool repeated) { defaultRepeated = repeated; }
    void setDefaultSmooth(bool smooth) { defaultSmooth = smooth; }
    void setDisabled(bool disable) { disabled = disable; }

    uint32_t getBaseMipLevel() const { return baseMipLevel; }
    bool isDefaultRepeated() { return defaultRepeated; }
    bool isDefaultSmoothFiltering() { return defaultSmooth; }

    sp::Texture* getTexture(const string& name);
private:
    sp::Texture* loadTexture(const string& name);
};

#endif//TEXTURE_MANAGER_H

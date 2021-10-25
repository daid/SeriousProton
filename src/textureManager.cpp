#include "logging.h"
#include "resources.h"
#include "textureManager.h"
#include "graphics/image.h"

TextureManager textureManager;

TextureManager::TextureManager()
{
    defaultRepeated = false;
    defaultSmooth = false;
    autoSprite = true;
    disabled = false;
}

TextureManager::~TextureManager()
{
}

sp::Texture* TextureManager::getTexture(const string& name)
{
    if (disabled)
        return nullptr;
    sp::Texture* data = textureMap[name];
    if (data == nullptr)
        return loadTexture(name);
    return data;
}

sp::Texture* TextureManager::loadTexture(const string& name)
{
    sp::BasicTexture* texture = new sp::BasicTexture();
    textureMap[name] = texture;

    sp::Image image;
    P<ResourceStream> stream = getResourceStream(name);
    if (!stream) stream = getResourceStream(name + ".png");
    if (!stream || !image.loadFromStream(stream))
    {
        LOG(WARNING) << "Failed to load texture: " << name;
        sp::Image backup_image({8, 8}, {255, 0, 255, 128});
        texture->loadFromImage(std::move(backup_image));
        return texture;
    }

    texture->setRepeated(defaultRepeated);
    texture->setSmooth(defaultSmooth);
    
    texture->loadFromImage(std::move(image));
    LOG(INFO) << "Loaded: " << name;
    return texture;
}

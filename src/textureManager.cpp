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

    P<ResourceStream> stream = getResourceStream(string(name) + ".ktx2");
    sp::Image image;
    if (stream)
    {
        image = sp::Texture::loadUASTC(stream, {});
        if (image.getSize().x == 0 || image.getSize().y == 0)
        {
            stream = nullptr;
        }
    }

    if (!stream)
    {
        stream = getResourceStream(name);
        if (!stream)
            stream = getResourceStream(string(name) + ".png");
        image.loadFromStream(stream);
    }

    if (image.getSize().x == 0 || image.getSize().y == 0)
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

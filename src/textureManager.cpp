#include "logging.h"
#include "resources.h"
#include "textureManager.h"
#include "graphics/image.h"
#include "graphics/ktx2texture.h"

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
    P<ResourceStream> stream;
    // filename variants:
    //  name
    //  name.notanextension
    //  name.ext
    //  name.notanext.ext
    // Attempt to load the best version.
    auto last_dot = name.find_last_of('.');
    if (last_dot != std::string::npos)
    {
        // Extension found, try and substitute it.
        stream = getResourceStream(name.substr(0, static_cast<uint32_t>(last_dot)) + ".ktx2");
    }

    if (!stream)
    {
        // No extension, or substitution failed (maybe it wasn't an extension), blindly add it.
        stream = getResourceStream(name + ".ktx2");
    }

    std::unique_ptr<sp::BasicTexture> texture;
    sp::KTX2Texture ktxtexture;
    if (stream)
    {
        if (ktxtexture.loadFromStream(stream))
        {
            texture = ktxtexture.toTexture(std::min(getBaseMipLevel(), ktxtexture.getMipCount() - 1));
            if (!texture)
                LOG(Warning, "[ktx2]: ", name, " failed to load into texture.");
        }
        else
            LOG(Warning, "[ktx2]: ", name, " failed to read stream.");
    }

    if (!texture)
    {
        sp::Image image;
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
            image = sp::Image({ 8, 8 }, { 255, 0, 255, 128 });
        }

        texture = std::make_unique<sp::BasicTexture>(image);
    }
    
    texture->setRepeated(defaultRepeated);
    texture->setSmooth(defaultSmooth);

    textureMap[name] = texture.get();
    LOG(INFO) << "Loaded: " << name;
    return texture.release();
}

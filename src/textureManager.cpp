#include "logging.h"
#include "resources.h"
#include "textureManager.h"

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

void TextureManager::setTexture(sf::Sprite& sprite, const string& name, unsigned int spriteIndex)
{
    TextureData& data = textureMap[name];
    if (data.texture.getSize().x < 1)
        loadTexture(name, sf::Vector2i(0, 0));
    
    if (spriteIndex < data.sprites.size())
    {
        sprite.setTexture(data.texture);
        sprite.setTextureRect(data.sprites[spriteIndex]);
        sprite.setOrigin(float(data.sprites[spriteIndex].width) / 2, float(data.sprites[spriteIndex].height) / 2);
    }else{
        sprite.setTexture(data.texture, true);
        sprite.setOrigin(data.texture.getSize().x / 2.f, data.texture.getSize().y / 2.f);
    }
}

sf::Texture* TextureManager::getTexture(const string& name, sf::Vector2i subDiv)
{
    if (disabled)
        return nullptr;
    TextureData& data = textureMap[name];
    if (data.texture.getSize().x < 1)
        loadTexture(name, subDiv);
    return &data.texture;
}

const sf::IntRect TextureManager::getSpriteRect(const string& name, unsigned int spriteIndex)
{
    TextureData& data = textureMap[name];
    if (data.texture.getSize().x < 1)
        loadTexture(name, sf::Vector2i(0, 0));
    
    if (spriteIndex < data.sprites.size())
        return data.sprites[spriteIndex];
    
    return sf::IntRect(sf::Vector2i(0, 0), sf::Vector2i(data.texture.getSize()));
}

void TextureManager::setSpriteRect(const string& name, unsigned int spriteIndex, const sf::IntRect rect)
{
    TextureData& data = textureMap[name];
    if (data.texture.getSize().x < 1)
        loadTexture(name, sf::Vector2i(0, 0));
    
    if (spriteIndex < data.sprites.size())
        data.sprites[spriteIndex] = rect;
    else
        data.sprites.push_back(rect);
}

void TextureManager::loadTexture(const string& name, sf::Vector2i subDiv)
{
    TextureData& data = textureMap[name];
    
    sf::Image tmpImage;
    P<ResourceStream> stream = getResourceStream(name);
    if (!stream) stream = getResourceStream(name + ".png");
    if (!stream || !tmpImage.loadFromStream(**stream))
    {
        LOG(WARNING) << "Failed to load texture: " << name;
        sf::Image image;
        image.create(8, 8, sf::Color(255, 0, 255, 128));
        data.texture.loadFromImage(image);
        return;
    }
    
    if (subDiv.x > 0 || subDiv.y > 0)
    {
        subDiv.x = std::max(subDiv.x, 1);
        subDiv.y = std::max(subDiv.y, 1);
        int w = tmpImage.getSize().x / subDiv.x;
        int h = tmpImage.getSize().y / subDiv.y;
        for(int y=0; y<subDiv.y; y++)
        {
            for(int x=0; x<subDiv.x; x++)
            {
                data.sprites.push_back(sf::IntRect(x*w, y*h, w, h));
            }
        }
    }else{
        if (autoSprite)
        {
            for(unsigned int y=0; y<tmpImage.getSize().y; y++)
            {
                for(unsigned int x=0; x<tmpImage.getSize().x; x++)
                {
                    if (tmpImage.getPixel(x, y) == sf::Color(255,0,255))
                    {
                        unsigned int x1=x+1;
                        for(; x1<tmpImage.getSize().x; x1++)
                            if (tmpImage.getPixel(x1, y) == sf::Color(255,0,255))
                                break;
                        unsigned int y1=y+1;
                        for(; y1<tmpImage.getSize().y; y1++)
                            if (tmpImage.getPixel(x, y1) == sf::Color(255,0,255))
                                break;
                        if (x1-x > 1 && y1-y > 1 && x1<tmpImage.getSize().x && y1<tmpImage.getSize().y)
                            data.sprites.push_back(sf::IntRect(x+1, y+1, x1-x-1, y1-y-1));
                        x = x1-1;
                    }
                }
            }
        }
    }
    data.texture.setRepeated(defaultRepeated);
    data.texture.setSmooth(defaultSmooth);
    
    data.texture.loadFromImage(tmpImage);
    LOG(INFO) << "Loaded: " << name << " with " << data.sprites.size() << " sprites";
}

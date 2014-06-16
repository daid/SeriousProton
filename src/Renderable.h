#ifndef RENDERABLE_H
#define RENDERABLE_H

#include <SFML/Graphics.hpp>
#include "P.h"

class Renderable;
class RenderLayer;
extern RenderLayer* defaultRenderLayer;

class RenderChain : public sf::NonCopyable
{
public:
    virtual void render(sf::RenderTarget& window) = 0;
};

class RenderLayer : public RenderChain
{
private:
    PVector<Renderable> renderableList;
    RenderChain* link;

public:
    RenderLayer();
    RenderLayer(RenderChain* link);
    
    virtual void render(sf::RenderTarget& window);
    
    friend class Renderable;
};

//Abstract class for entity that can be rendered.
class Renderable: public virtual PObject
{
    public:
        Renderable();
        Renderable(RenderLayer* renderLayer);
        virtual ~Renderable() {}
        virtual int preRender(sf::RenderTarget& window) {return 0;}
        virtual void render(sf::RenderTarget& window) = 0;
        virtual int postRender(sf::RenderTarget& window) {return 0;}
    protected:
    private:
};

#endif // RENDERABLE_H

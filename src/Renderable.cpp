#include "Renderable.h"

RenderLayer* defaultRenderLayer;

RenderLayer::RenderLayer()
: link(NULL)
{
}

RenderLayer::RenderLayer(RenderChain* link)
: link(link)
{
}

void RenderLayer::render(sf::RenderTarget& window)
{
    if (link)
        link->render(window);
    foreach(Renderable, r, renderableList)
        r->render(window);
}

Renderable::Renderable()
{
    defaultRenderLayer->renderableList.push_back(this);
}

Renderable::Renderable(RenderLayer* renderLayer)
{
    renderLayer->renderableList.push_back(this);
}

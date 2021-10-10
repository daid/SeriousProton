#include "Renderable.h"

RenderLayer* defaultRenderLayer;

RenderLayer::RenderLayer()
: link(NULL), active(true)
{
}

RenderLayer::RenderLayer(RenderChain* link)
: link(link), active(true)
{
}

void RenderLayer::render(sp::RenderTarget& target)
{
    if (link)
        link->render(target);
    if (active)
        foreach(Renderable, r, renderableList)
            r->render(target);
}

bool RenderLayer::onPointerMove(glm::vec2 position, int id)
{
    if (active)
        foreach(Renderable, r, renderableList)
            r->onPointerMove(position, id);
    if (link)
        return link->onPointerMove(position, id);
    return false;
}

void RenderLayer::onPointerLeave(int id)
{
    if (active)
        foreach(Renderable, r, renderableList)
            r->onPointerLeave(id);
    if (link)
        link->onPointerLeave(id);
}

bool RenderLayer::onPointerDown(sp::io::Pointer::Button button, glm::vec2 position, int id)
{
    if (active)
        foreach(Renderable, r, renderableList)
            r->onPointerDown(button, position, id);
    if (link)
        return link->onPointerDown(button, position, id);
    return false;
}

void RenderLayer::onPointerDrag(glm::vec2 position, int id)
{
    if (active)
        foreach(Renderable, r, renderableList)
            r->onPointerDrag(position, id);
    if (link)
        link->onPointerDrag(position, id);
}

void RenderLayer::onPointerUp(glm::vec2 position, int id)
{
    if (active)
        foreach(Renderable, r, renderableList)
            r->onPointerUp(position, id);
    if (link)
        link->onPointerUp(position, id);
}

void RenderLayer::onTextInput(const string& text)
{
    if (active)
        foreach(Renderable, r, renderableList)
            r->onTextInput(text);
    if (link)
        link->onTextInput(text);
}

void RenderLayer::onTextInput(sp::TextInputEvent e)
{
    if (active)
        foreach(Renderable, r, renderableList)
            r->onTextInput(e);
    if (link)
        link->onTextInput(e);
}


Renderable::Renderable()
{
    layer = defaultRenderLayer;
    if (layer)
        layer->renderableList.push_back(this);
}

Renderable::Renderable(RenderLayer* renderLayer)
{
    layer = renderLayer;
    if (layer)
        layer->renderableList.push_back(this);
}

Renderable::~Renderable()
{
}

void Renderable::moveToRenderLayer(RenderLayer* new_render_layer)
{
    if (layer)
        for(unsigned int n=0; n<layer->renderableList.size(); n++)
            if (layer->renderableList[n] == this)
                layer->renderableList[n] = NULL;
    layer = new_render_layer;
    if (layer)
        layer->renderableList.push_back(this);
}

#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include "P.h"
#include "Renderable.h"

class Window : public virtual PObject
{
private:
    glm::vec2 minimal_virtual_size;
    glm::vec2 current_virtual_size;
    void* window = nullptr;
    void* context = nullptr;
    RenderChain* renderChain;
    bool fullscreen;
    int fsaa;

public:
    Window(glm::vec2 virtual_size, bool fullscreen, RenderChain* chain, int fsaa = 0);
    virtual ~Window();

    glm::vec2 getVirtualSize() const { return current_virtual_size; }
    void render();

    bool isFullscreen() { return fullscreen; }
    void setFullscreen(bool fullscreen);
    int getFSAA() { return fsaa; }
    void setFSAA(int fsaa);

    void setTitle(string title);

    glm::vec2 mapPixelToCoords(glm::ivec2 point) const;
    glm::ivec2 mapCoordsToPixel(glm::vec2 point) const;

    friend class InputHandler;
    friend class Engine;
private:
    void create();
    void setupView();
};

#endif//WINDOW_MANAGER_H

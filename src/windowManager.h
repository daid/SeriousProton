#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include "P.h"
#include "Renderable.h"

class WindowManager : public virtual PObject
{
private:
    bool windowHasFocus;
    float min_aspect_ratio;
    bool allow_virtual_resize;

    glm::ivec2 virtualSize;
    void* window = nullptr;
    void* context = nullptr;
    RenderChain* renderChain;
    bool fullscreen;
    int fsaa;

    glm::mat4x4 viewport_matrix;
public:
    WindowManager(int virtualWidth, int virtualHeight, bool fullscreen, RenderChain* chain, int fsaa = 0);
    virtual ~WindowManager();

    glm::ivec2 getVirtualSize() const { return virtualSize; }
    void render();
    void close();
    bool hasFocus() { return windowHasFocus; }

    bool isFullscreen() { return fullscreen; }
    void setFullscreen(bool fullscreen);
    int getFSAA() { return fsaa; }
    void setFSAA(int fsaa);

    void setAllowVirtualResize(bool allow) { allow_virtual_resize = allow; setupView(); }
    void setFrameLimit(int limit);
    void setTitle(string title);

    glm::vec2 mapPixelToCoords(glm::ivec2 point) const;
    glm::ivec2 mapCoordsToPixel(glm::vec2 point) const;

    friend class InputHandler;
    friend class Engine;
    friend class Clipboard;

private:
    void create();
    void setupView();
};

#endif//WINDOW_MANAGER_H

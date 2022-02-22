#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include "P.h"
#include "Renderable.h"

union SDL_Event;
class Window : public virtual PObject
{
public:
    enum class Mode {
        Window,
        Fullscreen,
        ExclusiveFullscreen
    };
    Window(glm::vec2 virtual_size, Mode mode, RenderChain* chain, int fsaa = 0);
    virtual ~Window();

    glm::vec2 getVirtualSize() const { return current_virtual_size; }
    void render();

    Mode getMode() { return mode; }
    void setMode(Mode mode);
    int getFSAA() { return fsaa; }
    void setFSAA(int fsaa);

    void setTitle(string title);

    glm::vec2 mapPixelToCoords(glm::ivec2 point) const;
    glm::ivec2 mapCoordsToPixel(glm::vec2 point) const;

    friend class InputHandler;
    friend class Engine;
private:
    static PVector<Window> all_windows;
    static void* gl_context;

    glm::vec2 minimal_virtual_size;
    glm::vec2 current_virtual_size;
    void* window = nullptr;
    RenderChain* render_chain;
    int mouse_button_down_mask = 0;
    Mode mode;
    int fsaa;

    void handleEvent(const SDL_Event& event);
    void create();
    void setupView();
    glm::ivec2 calculateWindowSize() const;
};

#endif//WINDOW_MANAGER_H

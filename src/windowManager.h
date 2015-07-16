#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include <SFML/Graphics.hpp>
#include "P.h"
#include "Renderable.h"

class WindowManager : public virtual PObject
{
private:
    bool windowHasFocus;
    bool allow_horizontal_stretch;
    sf::Vector2i virtualSize;
    sf::RenderWindow window;
    RenderChain* renderChain;
    bool fullscreen;
    int fsaa;
public:
    WindowManager(int virtualWidth, int virtualHeight, bool fullscreen, RenderChain* chain, int fsaa = 0);
    virtual ~WindowManager();
    
    sf::Vector2i getVirtualSize() const { return virtualSize; }
    void render();
    void close();
    bool hasFocus() { return windowHasFocus; }
    
    bool isFullscreen() { return fullscreen; }
    void setFullscreen(bool fullscreen);
    int getFSAA() { return fsaa; }
    void setFSAA(int fsaa);

    friend class InputHandler;
    friend class Engine;

private:
    void create();
    void setupView();
};

#endif//WINDOW_MANAGER_H

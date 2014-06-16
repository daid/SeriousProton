#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include <SFML/Graphics.hpp>
#include "P.h"
#include "Renderable.h"

class WindowManager : public virtual PObject
{
private:
    bool windowHasFocus;
    sf::Vector2i virtualSize;
    sf::RenderWindow window;
    RenderChain* renderChain;
public:
    WindowManager(int virtualWidth, int virtualHeight, bool fullScreen, RenderChain* chain, int fsaa = 0);
    virtual ~WindowManager();
    
    sf::Vector2i getVirtualSize() const { return virtualSize; }
    void render();
    void close();
    bool hasFocus() { return windowHasFocus; }

    friend class InputHandler;
    friend class Engine;
};

#endif//WINDOW_MANAGER_H

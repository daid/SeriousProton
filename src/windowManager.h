#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#ifndef WINDOW_TITLE
#define WINDOW_TITLE "SeriousProton Game"
#endif // WINDOW_TITLE

#include <SFML/Graphics.hpp>
#include "P.h"
#include "Renderable.h"

class WindowManager : public virtual PObject
{
private:
    bool windowHasFocus;
    float min_aspect_ratio;
    bool allow_virtual_resize;

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

    void setAllowVirtualResize(bool allow) { allow_virtual_resize = allow; setupView(); }
    void setFrameLimit(int limit) { window.setFramerateLimit(limit); }

    sf::Vector2f mapPixelToCoords(const sf::Vector2i& point) const;
    sf::Vector2i mapCoordsToPixel(const sf::Vector2f& point) const;

    friend class InputHandler;
    friend class Engine;
    friend class Clipboard;
private:
    void create();
    void setupView();
};

#endif//WINDOW_MANAGER_H

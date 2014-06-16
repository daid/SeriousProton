#include "windowManager.h"
#include "Updatable.h"
#include "Renderable.h"
#include "Collisionable.h"
#include "postProcessManager.h"
#include "input.h"

WindowManager::WindowManager(int virtualWidth, int virtualHeight, bool fullscreen, RenderChain* renderChain, int fsaa)
: virtualSize(virtualWidth, virtualHeight), renderChain(renderChain)
{
    srand(time(NULL));
    windowHasFocus = true;

    // Create the window of the application
    int windowWidth = virtualWidth;
    int windowHeight = virtualHeight;
    
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    if (fullscreen)
    {
        windowWidth = desktop.width;
        windowHeight = desktop.height;
    }else{
        unsigned int scale = 2;
        while(windowWidth * scale < desktop.width && windowHeight * scale < desktop.height)
            scale += 1;
        windowWidth *= scale - 1;
        windowHeight *= scale - 1;
        
        while(windowWidth >= int(desktop.width) || windowHeight >= int(desktop.height))
        {
            windowWidth *= 0.8;
            windowHeight *= 0.8;
        }
        //windowHeight *= (1.33333 / 1.25);
    }

    if (fullscreen)
        window.create(sf::VideoMode(windowWidth, windowHeight, 32), "Game", sf::Style::Fullscreen, sf::ContextSettings(24, 8, fsaa, 2, 0));
    else
        window.create(sf::VideoMode(windowWidth, windowHeight, 32), "Game", sf::Style::None, sf::ContextSettings(24, 8, fsaa, 2, 0));
    window.setVerticalSyncEnabled(false);
    window.setFramerateLimit(60);
    window.setMouseCursorVisible(false);
    sf::View view(sf::Vector2f(virtualWidth/2,virtualHeight/2), sf::Vector2f(virtualWidth, virtualHeight));
    if (windowWidth * 1.1 > windowHeight * virtualWidth / virtualHeight)
    {
        float aspect = float(windowHeight) * float(virtualWidth) / float(virtualHeight) / float(windowWidth);
        float offset = 0.5 - 0.5 * aspect;
        view.setViewport(sf::FloatRect(offset, 0, aspect, 1));
    }else{
        float aspect = float(windowWidth) / float(windowHeight) * float(virtualHeight) / float(virtualWidth);
        float offset = 0.5 - 0.5 * aspect;
        view.setViewport(sf::FloatRect(0, offset, 1, aspect));
    }
    window.setView(view);
}

WindowManager::~WindowManager()
{
}

void WindowManager::render()
{
    // Clear the window
    window.clear(sf::Color(20, 20, 20));
    
    //Call the first item of the rendering chain.
    renderChain->render(window);

    // Display things on screen
    window.display();
}

void WindowManager::close()
{
    window.close();
}

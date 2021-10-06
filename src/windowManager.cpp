#include "windowManager.h"
#include "graphics/opengl.h"
#include "Updatable.h"
#include "Renderable.h"
#include "collisionable.h"
#include "postProcessManager.h"
#include "input.h"

#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <SDL.h>

#ifdef _WIN32
#include <windows.h>
#endif

Window::Window(glm::vec2 virtual_size, bool fullscreen, RenderChain* renderChain, int fsaa)
: minimal_virtual_size(virtual_size), current_virtual_size(virtual_size), renderChain(renderChain), fullscreen(fullscreen), fsaa(fsaa)
{
    srand(static_cast<int32_t>(time(nullptr)));

#ifdef _WIN32
    //On Vista or newer windows, let the OS know we are DPI aware, so we won't have odd scaling issues.
    HINSTANCE user_dll = LoadLibrary("USER32.DLL");
    if (user_dll)
    {
        BOOL(WINAPI *SetProcessDPIAware)(void);
        SetProcessDPIAware = reinterpret_cast<BOOL(WINAPI *)(void)>(GetProcAddress(user_dll, "SetProcessDPIAware"));
        if (SetProcessDPIAware)
            SetProcessDPIAware();
    }
    FreeLibrary(user_dll);
#endif

    create();
    sp::initOpenGL();
}

Window::~Window()
{
}

void Window::render()
{
#warning SDL2 TODO
/*
    if (InputHandler::keyboardIsPressed(sf::Keyboard::Return) && (sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) || sf::Keyboard::isKeyPressed(sf::Keyboard::RAlt)))
    {
        setFullscreen(!isFullscreen());
    }
*/

    // Clear the window
    glClearColor(0.1, 0.1, 0.1, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int w, h;
    SDL_GetWindowSize(static_cast<SDL_Window*>(window), &w, &h);
    glViewport(0, 0, w, h);

    //Call the first item of the rendering chain.
    sp::RenderTarget target{current_virtual_size, {w, h}};
    renderChain->render(target);
    target.finish();

    // Display things on screen
    SDL_GL_SwapWindow(static_cast<SDL_Window*>(window));
}

void Window::setFullscreen(bool new_fullscreen)
{
    if (fullscreen == new_fullscreen)
        return;
    fullscreen = new_fullscreen;
    create();
}
void Window::setFSAA(int new_fsaa)
{
    if (fsaa == new_fsaa)
        return;
    fsaa = new_fsaa;
    create();
}

void Window::setTitle(string title)
{
    SDL_SetWindowTitle(static_cast<SDL_Window*>(window), title.c_str());
}

glm::vec2 Window::mapPixelToCoords(const glm::ivec2 point) const
{
    int w, h;
    SDL_GetWindowSize(static_cast<SDL_Window*>(window), &w, &h);
    float x = float(point.x) / float(w) * float(current_virtual_size.x);
    float y = float(point.y) / float(h) * float(current_virtual_size.y);
    return glm::vec2(x, y);
}

glm::ivec2 Window::mapCoordsToPixel(const glm::vec2 point) const
{
    int w, h;
    SDL_GetWindowSize(static_cast<SDL_Window*>(window), &w, &h);
    float x = float(point.x) * float(w) / float(current_virtual_size.x);
    float y = float(point.y) * float(h) / float(current_virtual_size.y);
    return glm::ivec2(x, y);
}

void Window::create()
{
    if (window) return;

    // Create the window of the application
    int windowWidth = minimal_virtual_size.x;
    int windowHeight = minimal_virtual_size.y;

    SDL_Rect rect;
    SDL_GetDisplayBounds(0, &rect);
    if (fullscreen)
    {
        windowWidth = rect.w;
        windowHeight = rect.h;
    }else{
        int scale = 2;
        while(windowWidth * scale < int(rect.w) && windowHeight * scale < int(rect.h))
            scale += 1;
        windowWidth *= scale - 1;
        windowHeight *= scale - 1;

        while(windowWidth >= int(rect.w) || windowHeight >= int(rect.h) - 100)
        {
            windowWidth = static_cast<int>(std::floor(windowWidth * 0.9f));
            windowHeight = static_cast<int>(std::floor(windowHeight * 0.9f));
        }
    }

    int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    if (fullscreen)
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    window = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, flags);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    context = SDL_GL_CreateContext(static_cast<SDL_Window*>(window));
    if (SDL_GL_SetSwapInterval(-1))
        SDL_GL_SetSwapInterval(1);
    setupView();
}

void Window::setupView()
{
    int w, h;
    SDL_GetWindowSize(static_cast<SDL_Window*>(window), &w, &h);
    glm::vec2 window_size{w, h};

    current_virtual_size = minimal_virtual_size;

    if (window_size.x / window_size.y > current_virtual_size.x / current_virtual_size.y)
    {
        current_virtual_size.x = current_virtual_size.y / window_size.y * window_size.x;
    }else{
        current_virtual_size.y = current_virtual_size.x / window_size.x * window_size.y;
    }
    LOG(Debug, window_size.x / window_size.y, " ", current_virtual_size.x / current_virtual_size.y);
    LOG(Debug, window_size.x, " ", window_size.y, " ",current_virtual_size.x, " ", current_virtual_size.y);
}

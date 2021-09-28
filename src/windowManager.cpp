#include <GL/glew.h>
#include "windowManager.h"
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

WindowManager::WindowManager(int virtualWidth, int virtualHeight, bool fullscreen, RenderChain* renderChain, int fsaa)
: virtualSize(virtualWidth, virtualHeight), renderChain(renderChain), fullscreen(fullscreen), fsaa(fsaa)
{
    srand(static_cast<int32_t>(time(nullptr)));
    windowHasFocus = true;
    min_aspect_ratio = float(virtualWidth) / float(virtualHeight);
    allow_virtual_resize = false;

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
    glewInit();
}

WindowManager::~WindowManager()
{
}

void WindowManager::render()
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
    glLoadIdentity();
    glLoadMatrixf(glm::value_ptr(viewport_matrix));

    int w, h;
    SDL_GetWindowSize(static_cast<SDL_Window*>(window), &w, &h);

    //Call the first item of the rendering chain.
    sp::RenderTarget target{virtualSize, {w, h}};
    renderChain->render(target);
    target.finish();

    // Display things on screen
    SDL_GL_SwapWindow(static_cast<SDL_Window*>(window));
}

void WindowManager::close()
{
#warning SDL2 TODO
}

void WindowManager::setFullscreen(bool new_fullscreen)
{
    if (fullscreen == new_fullscreen)
        return;
    fullscreen = new_fullscreen;
    create();
}
void WindowManager::setFSAA(int new_fsaa)
{
    if (fsaa == new_fsaa)
        return;
    fsaa = new_fsaa;
    create();
}

void WindowManager::setFrameLimit(int limit)
{
    //TODO
}

void WindowManager::setTitle(string title)
{
    SDL_SetWindowTitle(static_cast<SDL_Window*>(window), title.c_str());
}

glm::vec2 WindowManager::mapPixelToCoords(const glm::ivec2 point) const
{
    int w, h;
    SDL_GetWindowSize(static_cast<SDL_Window*>(window), &w, &h);
    float x = float(point.x) / float(w) * float(virtualSize.x);
    float y = float(point.y) / float(h) * float(virtualSize.y);
    return glm::vec2(x, y);
}

glm::ivec2 WindowManager::mapCoordsToPixel(const glm::vec2 point) const
{
#warning SDL2 TODO
    return glm::ivec2(0, 0);
}

void WindowManager::create()
{
    if (window) return;

    // Create the window of the application
    int windowWidth = virtualSize.x;
    int windowHeight = virtualSize.y;

    SDL_Rect rect;
    SDL_GetDisplayBounds(0, &rect);
    if (fullscreen)
    {
        windowWidth = rect.w;
        windowHeight = rect.h;
    }else{
        unsigned int scale = 2;
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
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 2);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    context = SDL_GL_CreateContext(static_cast<SDL_Window*>(window));
    if (SDL_GL_SetSwapInterval(-1))
        SDL_GL_SetSwapInterval(1);
    setupView();
}

void WindowManager::setupView()
{
    int w, h;
    SDL_GetWindowSize(static_cast<SDL_Window*>(window), &w, &h);
    glm::vec2 window_size{w, h};
    if (window_size.x / window_size.y > min_aspect_ratio)
    {
        if (allow_virtual_resize)
            virtualSize.x = static_cast<int32_t>(virtualSize.y * (window_size.x / window_size.y));

        float aspect = window_size.y * float(virtualSize.x) / float(virtualSize.y) / window_size.x;
        float offset = 0;//0.5 - 0.5 * aspect;
    }else{
        virtualSize.x = static_cast<int32_t>(virtualSize.y * min_aspect_ratio);

        float aspect = window_size.x / window_size.y * float(virtualSize.y) / float(virtualSize.x);
        float offset = 0.5f - 0.5f * aspect;
        //TODO: Build viewport matrix
    }

    viewport_matrix = glm::orthoLH(0.0f, float(virtualSize.x), float(virtualSize.y), 0.0f, 0.0f, 1.0f);
}

#include "windowManager.h"

#ifdef WIN32
#include "dynamicLibrary.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "graphics/opengl.h"
#include "Updatable.h"
#include "Renderable.h"
#include "collisionable.h"
#include "postProcessManager.h"

#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <SDL.h>

PVector<Window> Window::all_windows;
void* Window::gl_context = nullptr;

Window::Window(glm::vec2 virtual_size, bool fullscreen, RenderChain* render_chain, int fsaa)
: minimal_virtual_size(virtual_size), current_virtual_size(virtual_size), render_chain(render_chain), fullscreen(fullscreen), fsaa(fsaa)
{
    srand(static_cast<int32_t>(time(nullptr)));

#ifdef _WIN32
    //On Vista or newer windows, let the OS know we are DPI aware, so we won't have odd scaling issues.
    auto user32 = DynamicLibrary::open("USER32.DLL");
    if (user32)
    {
        auto SetProcessDPIAware = user32->getFunction<BOOL(WINAPI *)(void)>("SetProcessDPIAware");
        if (SetProcessDPIAware)
            SetProcessDPIAware();
    }
#endif

    create();
    sp::initOpenGL();

    all_windows.push_back(this);
}

Window::~Window()
{

    if (gl_context && all_windows.size() <= 1)
        SDL_GL_DeleteContext(gl_context);
    if (window)
        SDL_DestroyWindow(static_cast<SDL_Window*>(window));
}

void Window::render()
{
    SDL_GL_MakeCurrent(static_cast<SDL_Window*>(window), gl_context);

    int w, h;
    SDL_GL_GetDrawableSize(static_cast<SDL_Window*>(window), &w, &h);
    glViewport(0, 0, w, h);

    // Clear the window
    glClearColor(0.1f, 0.1f, 0.1f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //Call the first item of the rendering chain.
    sp::RenderTarget target{current_virtual_size, {w, h}};
    render_chain->render(target);
    target.finish();

    // Display things on screen
    SDL_GL_SwapWindow(static_cast<SDL_Window*>(window));
}

void Window::setFullscreen(bool new_fullscreen)
{
    if (fullscreen == new_fullscreen)
        return;
    fullscreen = new_fullscreen;
    SDL_SetWindowFullscreen(static_cast<SDL_Window*>(window), fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    setupView();
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

    int display_nr = 0;
    for(auto w : all_windows)
    {
        if (w == this)
            break;
        display_nr ++;
    }

    // Create the window of the application
    auto windowWidth = static_cast<int>(minimal_virtual_size.x);
    auto windowHeight = static_cast<int>(minimal_virtual_size.y);

    SDL_Rect rect;
    if (SDL_GetDisplayBounds(display_nr, &rect))
    {
        display_nr = 0;
        SDL_GetDisplayBounds(display_nr, &rect);
    }

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

#if defined(ANDROID)
    constexpr auto context_profile_mask = SDL_GL_CONTEXT_PROFILE_ES;
    constexpr auto context_profile_minor_version = 0;

#else
    constexpr auto context_profile_mask = SDL_GL_CONTEXT_PROFILE_CORE;
    constexpr auto context_profile_minor_version = 1;
#endif

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, context_profile_mask);
#if defined(DEBUG)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, context_profile_minor_version);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    if (fullscreen)
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    window = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED_DISPLAY(display_nr), SDL_WINDOWPOS_CENTERED_DISPLAY(display_nr), windowWidth, windowHeight, flags);
    if (!gl_context)
        gl_context = SDL_GL_CreateContext(static_cast<SDL_Window*>(window));
    if (SDL_GL_SetSwapInterval(-1))
        SDL_GL_SetSwapInterval(1);
    setupView();
}

void Window::handleEvent(const SDL_Event& event)
{
    switch(event.type)
    {
    case SDL_MOUSEBUTTONDOWN:
        {
            sp::io::Pointer::Button button = sp::io::Pointer::Button::Unknown;
            switch(event.button.button)
            {
            case SDL_BUTTON_LEFT: button = sp::io::Pointer::Button::Left; break;
            case SDL_BUTTON_MIDDLE: button = sp::io::Pointer::Button::Middle; break;
            case SDL_BUTTON_RIGHT: button = sp::io::Pointer::Button::Right; break;
            default: break;
            }
            mouse_button_down_mask |= 1 << int(event.button.button);
            render_chain->onPointerDown(button, mapPixelToCoords({event.button.x, event.button.y}), sp::io::Pointer::mouse);
        }
        break;
    case SDL_MOUSEMOTION:
        if (mouse_button_down_mask)
            render_chain->onPointerDrag(mapPixelToCoords({event.motion.x, event.motion.y}), sp::io::Pointer::mouse);
        else
            render_chain->onPointerMove(mapPixelToCoords({event.motion.x, event.motion.y}), sp::io::Pointer::mouse);
        break;
    case SDL_MOUSEBUTTONUP:
        mouse_button_down_mask &=~(1 << int(event.button.button));
        if (!mouse_button_down_mask)
        {
            render_chain->onPointerUp(mapPixelToCoords({event.button.x, event.button.y}), sp::io::Pointer::mouse);
            render_chain->onPointerMove(mapPixelToCoords({event.button.x, event.button.y}), sp::io::Pointer::mouse);
        }
        break;
    case SDL_FINGERDOWN:
        render_chain->onPointerDown(sp::io::Pointer::Button::Touch, {event.tfinger.x * current_virtual_size.x, event.tfinger.y * current_virtual_size.y}, event.tfinger.fingerId);
        break;
    case SDL_FINGERMOTION:
        render_chain->onPointerDrag({event.tfinger.x * current_virtual_size.x, event.tfinger.y * current_virtual_size.y}, event.tfinger.fingerId);
        break;
    case SDL_FINGERUP:
        render_chain->onPointerUp({event.tfinger.x * current_virtual_size.x, event.tfinger.y * current_virtual_size.y}, event.tfinger.fingerId);
        break;
    case SDL_TEXTINPUT:
        render_chain->onTextInput(event.text.text);
        break;
    case SDL_KEYDOWN:
        switch(event.key.keysym.sym)
        {
        case SDLK_KP_4:
            if (event.key.keysym.mod & KMOD_NUM)
                break;
            //fallthrough
        case SDLK_LEFT:
            if (event.key.keysym.mod & KMOD_SHIFT && event.key.keysym.mod & KMOD_CTRL)
                render_chain->onTextInput(sp::TextInputEvent::WordLeftWithSelection);
            else if (event.key.keysym.mod & KMOD_CTRL)
                render_chain->onTextInput(sp::TextInputEvent::WordLeft);
            else if (event.key.keysym.mod & KMOD_SHIFT)
                render_chain->onTextInput(sp::TextInputEvent::LeftWithSelection);
            else
                render_chain->onTextInput(sp::TextInputEvent::Left);
            break;
        case SDLK_KP_6:
            if (event.key.keysym.mod & KMOD_NUM)
                break;
            //fallthrough
        case SDLK_RIGHT:
            if (event.key.keysym.mod & KMOD_SHIFT && event.key.keysym.mod & KMOD_CTRL)
                render_chain->onTextInput(sp::TextInputEvent::WordRightWithSelection);
            else if (event.key.keysym.mod & KMOD_CTRL)
                render_chain->onTextInput(sp::TextInputEvent::WordRight);
            else if (event.key.keysym.mod & KMOD_SHIFT)
                render_chain->onTextInput(sp::TextInputEvent::RightWithSelection);
            else
                render_chain->onTextInput(sp::TextInputEvent::Right);
            break;
        case SDLK_KP_8:
            if (event.key.keysym.mod & KMOD_NUM)
                break;
            //fallthrough
        case SDLK_UP:
            if (event.key.keysym.mod & KMOD_SHIFT)
                render_chain->onTextInput(sp::TextInputEvent::UpWithSelection);
            else
                render_chain->onTextInput(sp::TextInputEvent::Up);
            break;
        case SDLK_KP_2:
            if (event.key.keysym.mod & KMOD_NUM)
                break;
            //fallthrough
        case SDLK_DOWN:
            if (event.key.keysym.mod & KMOD_SHIFT)
                render_chain->onTextInput(sp::TextInputEvent::DownWithSelection);
            else
                render_chain->onTextInput(sp::TextInputEvent::Down);
            break;
        case SDLK_KP_7:
            if (event.key.keysym.mod & KMOD_NUM)
                break;
            //fallthrough
        case SDLK_HOME:
            if (event.key.keysym.mod & KMOD_SHIFT && event.key.keysym.mod & KMOD_CTRL)
                render_chain->onTextInput(sp::TextInputEvent::TextStartWithSelection);
            else if (event.key.keysym.mod & KMOD_CTRL)
                render_chain->onTextInput(sp::TextInputEvent::TextStart);
            else if (event.key.keysym.mod & KMOD_SHIFT)
                render_chain->onTextInput(sp::TextInputEvent::LineStartWithSelection);
            else
                render_chain->onTextInput(sp::TextInputEvent::LineStart);
            break;
        case SDLK_KP_1:
            if (event.key.keysym.mod & KMOD_NUM)
                break;
            //fallthrough
        case SDLK_END:
            if (event.key.keysym.mod & KMOD_SHIFT && event.key.keysym.mod & KMOD_CTRL)
                render_chain->onTextInput(sp::TextInputEvent::TextEndWithSelection);
            else if (event.key.keysym.mod & KMOD_CTRL)
                render_chain->onTextInput(sp::TextInputEvent::TextEnd);
            else if (event.key.keysym.mod & KMOD_SHIFT)
                render_chain->onTextInput(sp::TextInputEvent::LineEndWithSelection);
            else
                render_chain->onTextInput(sp::TextInputEvent::LineEnd);
            break;
        case SDLK_KP_PERIOD:
            if (event.key.keysym.mod & KMOD_NUM)
                break;
            //fallthrough
        case SDLK_DELETE:
            render_chain->onTextInput(sp::TextInputEvent::Delete);
            break;
        case SDLK_BACKSPACE:
            render_chain->onTextInput(sp::TextInputEvent::Backspace);
            break;
        case SDLK_KP_ENTER:
        case SDLK_RETURN:
            if (event.key.keysym.mod & KMOD_ALT)
                setFullscreen(!isFullscreen());
            else
                render_chain->onTextInput(sp::TextInputEvent::Return);
            break;
        case SDLK_TAB:
        case SDLK_KP_TAB:
            if (event.key.keysym.mod & KMOD_SHIFT)
                render_chain->onTextInput(sp::TextInputEvent::Unindent);
            else
                render_chain->onTextInput(sp::TextInputEvent::Indent);
            break;
        case SDLK_a:
            if (event.key.keysym.mod & KMOD_CTRL)
                render_chain->onTextInput(sp::TextInputEvent::SelectAll);
            break;
        case SDLK_c:
            if (event.key.keysym.mod & KMOD_CTRL)
                render_chain->onTextInput(sp::TextInputEvent::Copy);
            break;
        case SDLK_v:
            if (event.key.keysym.mod & KMOD_CTRL)
                render_chain->onTextInput(sp::TextInputEvent::Paste);
            break;
        case SDLK_x:
            if (event.key.keysym.mod & KMOD_CTRL)
                render_chain->onTextInput(sp::TextInputEvent::Cut);
            break;
        }
        break;
    case SDL_WINDOWEVENT:
        switch(event.window.event)
        {
        case SDL_WINDOWEVENT_LEAVE:
            if (!SDL_GetMouseState(nullptr, nullptr))
            {
                render_chain->onPointerLeave(-1);
            }
            break;
        case SDL_WINDOWEVENT_CLOSE:
            //close();
            break;
        case SDL_WINDOWEVENT_RESIZED:
            setupView();
            break;
        }
        break;
    case SDL_QUIT:
        //close();
        break;
    default:
        break;
    }
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
}

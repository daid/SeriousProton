#include "input.h"
#include "engine.h"

P<WindowManager> InputHandler::windowManager;
bool InputHandler::touch_screen = false;
bool InputHandler::swap_xy = false;

sf::Vector2f InputHandler::mousePos;
int InputHandler::mouse_wheel_delta;
bool InputHandler::mouse_button_down[sf::Mouse::ButtonCount];
bool InputHandler::keyboard_button_down[sf::Keyboard::KeyCount];

bool InputHandler::mouseButtonDown[sf::Mouse::ButtonCount];
bool InputHandler::mouseButtonPressed[sf::Mouse::ButtonCount];
bool InputHandler::mouseButtonReleased[sf::Mouse::ButtonCount];
bool InputHandler::keyboardButtonDown[sf::Keyboard::KeyCount];
bool InputHandler::keyboardButtonPressed[sf::Keyboard::KeyCount];
bool InputHandler::keyboardButtonReleased[sf::Keyboard::KeyCount];
string InputHandler::keyboard_text_entry;

void InputHandler::initialize()
{
    memset(mouse_button_down, 0, sizeof(mouse_button_down));
    memset(keyboard_button_down, 0, sizeof(keyboard_button_down));
}

void InputHandler::update()
{
    if (!windowManager)
        windowManager = engine->getObject("windowManager");

    mousePos = sf::Vector2f(sf::Mouse::getPosition());
    if (swap_xy)
    {
        sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
        float x = mousePos.y * desktop.width / desktop.height;
        float y = mousePos.x * desktop.height / desktop.width;
        mousePos.x = x;
        mousePos.y = y;
    }
    mousePos = mousePos - sf::Vector2f(windowManager->window.getPosition());
    mousePos.x *= float(windowManager->virtualSize.x) / float(windowManager->window.getSize().x);
    mousePos.y *= float(windowManager->virtualSize.y) / float(windowManager->window.getSize().y);
    for(unsigned int n=0; n<sf::Mouse::ButtonCount; n++)
    {
        //bool down = sf::Mouse::isButtonPressed(sf::Mouse::Button(n)) && windowManager->hasFocus();    //
        bool down = mouse_button_down[n];
        mouseButtonPressed[n] = (!mouseButtonDown[n] && down);
        mouseButtonReleased[n] = (mouseButtonDown[n] && !down);
        mouseButtonDown[n] = down;
    }
    
    for(unsigned int n=0; n<sf::Keyboard::KeyCount; n++)
    {
        bool down = keyboard_button_down[n];
        keyboardButtonPressed[n] = (!keyboardButtonDown[n] && down);
        keyboardButtonReleased[n] = (keyboardButtonDown[n] && !down);
        keyboardButtonDown[n] = down;
    }
    
    if (touch_screen)
    {
        bool any_button_down = false;
        for(unsigned int n=0; n<sf::Mouse::ButtonCount; n++)
            if (mouseButtonDown[n] || mouseButtonReleased[n])
                any_button_down = true;
        if (!any_button_down)
        {
            mousePos.x = -1;
            mousePos.y = -1;
        }
    }
}

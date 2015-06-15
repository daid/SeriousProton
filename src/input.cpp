#include "input.h"
#include "engine.h"

P<WindowManager> InputHandler::windowManager;
bool InputHandler::touch_screen = false;
sf::Transform InputHandler::mouse_transform;
PVector<InputEventHandler> InputHandler::input_event_handlers;

sf::Vector2f InputHandler::mousePos;
float InputHandler::mouse_wheel_delta;
bool InputHandler::mouse_button_down[sf::Mouse::ButtonCount];
bool InputHandler::keyboard_button_down[sf::Keyboard::KeyCount];

bool InputHandler::mouseButtonDown[sf::Mouse::ButtonCount];
bool InputHandler::mouseButtonPressed[sf::Mouse::ButtonCount];
bool InputHandler::mouseButtonReleased[sf::Mouse::ButtonCount];
bool InputHandler::keyboardButtonDown[sf::Keyboard::KeyCount];
bool InputHandler::keyboardButtonPressed[sf::Keyboard::KeyCount];
bool InputHandler::keyboardButtonReleased[sf::Keyboard::KeyCount];

InputEventHandler::InputEventHandler()
{
    InputHandler::input_event_handlers.push_back(this);
}

InputEventHandler::~InputEventHandler()
{
}

void InputEventHandler::handleKeyPress(sf::Keyboard::Key key, int unicode)
{
}

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
    mousePos = mousePos - sf::Vector2f(windowManager->window.getPosition());
    mousePos.x *= float(windowManager->virtualSize.x) / float(windowManager->window.getSize().x);
    mousePos.y *= float(windowManager->virtualSize.y) / float(windowManager->window.getSize().y);
    mousePos = mouse_transform.transformPoint(mousePos);
    for(unsigned int n=0; n<sf::Mouse::ButtonCount; n++)
    {
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

void InputHandler::fireKeyEvent(sf::Keyboard::Key key, int unicode)
{
    foreach(InputEventHandler, e, input_event_handlers)
    {
        e->handleKeyPress(key, unicode);
    }
}

#include "input.h"
#include "engine.h"

P<WindowManager> InputHandler::windowManager;
bool InputHandler::touch_screen = false;
bool InputHandler::joystick = false;
sf::Transform InputHandler::mouse_transform;
PVector<InputEventHandler> InputHandler::input_event_handlers;

sf::Vector2f InputHandler::mousePos;
float InputHandler::mouse_wheel_delta;
bool InputHandler::mouse_button_down[sf::Mouse::ButtonCount];
bool InputHandler::keyboard_button_down[sf::Keyboard::KeyCount];

sf::Vector2f InputHandler::joystick_pos_xy = sf::Vector2f();
float InputHandler::joystick_pos_z = 0.0f;
float InputHandler::joystick_pos_r = 0.0f;
float InputHandler::joystick_xy_delta;
bool InputHandler::joystick_button_down[sf::Joystick::ButtonCount];
float InputHandler::joystick_axis_pos[sf::Joystick::AxisCount];

bool InputHandler::mouseButtonDown[sf::Mouse::ButtonCount];
bool InputHandler::mouseButtonPressed[sf::Mouse::ButtonCount];
bool InputHandler::mouseButtonReleased[sf::Mouse::ButtonCount];
bool InputHandler::keyboardButtonDown[sf::Keyboard::KeyCount];
bool InputHandler::keyboardButtonPressed[sf::Keyboard::KeyCount];
bool InputHandler::keyboardButtonReleased[sf::Keyboard::KeyCount];
bool InputHandler::joystickButtonDown[sf::Joystick::ButtonCount];
bool InputHandler::joystickButtonPressed[sf::Joystick::ButtonCount];
bool InputHandler::joystickButtonReleased[sf::Joystick::ButtonCount];

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
    memset(joystick_button_down, 0, sizeof(joystick_button_down));
    memset(joystick_axis_pos, 0, sizeof(joystick_axis_pos));
#ifdef __ANDROID__
    touch_screen = true;
#endif
    joystick = sf::Joystick::isConnected(0);
}

void InputHandler::update()
{
    if (!windowManager)
        windowManager = engine->getObject("windowManager");

    for(unsigned int n=0; n<sf::Keyboard::KeyCount; n++)
    {
        bool down = keyboard_button_down[n];
        keyboardButtonPressed[n] = (!keyboardButtonDown[n] && down);
        keyboardButtonReleased[n] = (keyboardButtonDown[n] && !down);
        keyboardButtonDown[n] = down;
    }

#ifdef __ANDROID__
    if (sf::Touch::isDown(0))
    {
        mousePos = realWindowPosToVirtual(sf::Touch::getPosition(0));
        mouse_button_down[sf::Mouse::Left] = true;
    }else{
        mouse_button_down[sf::Mouse::Left] = false;
    }
#else
    mousePos = realWindowPosToVirtual(sf::Mouse::getPosition(windowManager->window));
#endif
    mousePos = mouse_transform.transformPoint(mousePos);

    for(unsigned int n=0; n<sf::Mouse::ButtonCount; n++)
    {
        bool down = mouse_button_down[n];
        mouseButtonPressed[n] = (!mouseButtonDown[n] && down);
        mouseButtonReleased[n] = (mouseButtonDown[n] && !down);
        mouseButtonDown[n] = down;
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
    
    if (sf::Joystick::isConnected(0))
    {
        sf::Vector2f joystick_position;
        joystick_position.x = sf::Joystick::getAxisPosition(0, sf::Joystick::X);
        joystick_position.y = sf::Joystick::getAxisPosition(0, sf::Joystick::Y);
        if (sqrt((joystick_position.x * joystick_position.x) + 
                 (joystick_position.y * joystick_position.y)) 
            > joystick_xy_hysteresis)
        {
            if (joystick_position.x > 0.0)
                joystick_pos_xy.x = (joystick_position.x - joystick_xy_hysteresis) * ((joystick_xy_hysteresis / 100) + 1);
            if (joystick_position.x < 0.0)
                joystick_pos_xy.x = (joystick_position.x + joystick_xy_hysteresis) * ((joystick_xy_hysteresis / 100) + 1);
            if (joystick_position.y > 0.0)
                joystick_pos_xy.y = (joystick_position.y - joystick_xy_hysteresis) * ((joystick_xy_hysteresis / 100) + 1);
            if (joystick_position.y < 0.0)
                joystick_pos_xy.y = (joystick_position.y + joystick_xy_hysteresis) * ((joystick_xy_hysteresis / 100) + 1);
        }
        else
        {
            joystick_pos_xy = sf::Vector2f(0.0f, 0.0f);
        }
            
        float axis_pos;
        axis_pos = sf::Joystick::getAxisPosition(0, sf::Joystick::Z);
        if (fabs(axis_pos) > joystick_z_hysteresis)
            if (axis_pos > 0.0) // retain resolution
                joystick_pos_z = (axis_pos - joystick_z_hysteresis) * ((joystick_z_hysteresis / 100) + 1);
            else
                joystick_pos_z = (axis_pos + joystick_z_hysteresis) * ((joystick_z_hysteresis / 100) + 1);
        else
            joystick_pos_z = 0.0;
            
        axis_pos = sf::Joystick::getAxisPosition(0, sf::Joystick::R);
        if (fabs(axis_pos) > joystick_r_hysteresis)
            if (axis_pos > 0.0) // retain resolution
                joystick_pos_r = (axis_pos - joystick_r_hysteresis) * ((joystick_r_hysteresis / 100) + 1);
            else
                joystick_pos_r = (axis_pos + joystick_r_hysteresis) * ((joystick_r_hysteresis / 100) + 1);
        else
            joystick_pos_r = 0.0;

        for(unsigned int n=0; n<sf::Joystick::ButtonCount; n++)
        {
            bool down = joystick_button_down[n];
            joystickButtonPressed[n] = (!joystickButtonDown[n] && down);
            joystickButtonReleased[n] = (joystickButtonDown[n] && !down);
            joystickButtonDown[n] = down;
        }
    }
}

void InputHandler::setMousePos(sf::Vector2f position)
{
    if (!windowManager)
        windowManager = engine->getObject("windowManager");

    sf::Mouse::setPosition(virtualWindowPosToReal(position), windowManager->window);
    mousePos = realWindowPosToVirtual(sf::Mouse::getPosition(windowManager->window));
}

void InputHandler::fireKeyEvent(sf::Keyboard::Key key, int unicode)
{
    foreach(InputEventHandler, e, input_event_handlers)
    {
        e->handleKeyPress(key, unicode);
    }
}

sf::Vector2f InputHandler::realWindowPosToVirtual(sf::Vector2i position)
{
    sf::FloatRect viewport = windowManager->window.getView().getViewport();
    sf::Vector2f pos = sf::Vector2f(position);
    
    pos.x -= viewport.left * float(windowManager->window.getSize().x);
    pos.y -= viewport.top * float(windowManager->window.getSize().y);
    pos.x *= float(windowManager->virtualSize.x) / float(windowManager->window.getSize().x) / viewport.width;
    pos.y *= float(windowManager->virtualSize.y) / float(windowManager->window.getSize().y) / viewport.height;
    return pos;
}

sf::Vector2i InputHandler::virtualWindowPosToReal(sf::Vector2f position)
{
    sf::FloatRect viewport = windowManager->window.getView().getViewport();

    position.x /= float(windowManager->virtualSize.x) / float(windowManager->window.getSize().x) / viewport.width;
    position.y /= float(windowManager->virtualSize.y) / float(windowManager->window.getSize().y) / viewport.height;
    
    position.x += viewport.left * float(windowManager->window.getSize().x);
    position.y += viewport.top * float(windowManager->window.getSize().y);
    return sf::Vector2i(position);
}

#include "input.h"
#include "engine.h"

P<WindowManager> InputHandler::windowManager;
bool InputHandler::touch_screen = false;
sf::Transform InputHandler::mouse_transform;
PVector<InputEventHandler> InputHandler::input_event_handlers;

bool InputHandler::keyboard_button_down[sf::Keyboard::KeyCount];
bool InputHandler::keyboard_button_pressed[sf::Keyboard::KeyCount];
bool InputHandler::keyboard_button_released[sf::Keyboard::KeyCount];
sf::Event::KeyEvent InputHandler::last_key_press;

sf::Vector2f InputHandler::mouse_position;
float InputHandler::mouse_wheel_delta;
bool InputHandler::mouse_button_down[sf::Mouse::ButtonCount];
bool InputHandler::mouse_button_pressed[sf::Mouse::ButtonCount];
bool InputHandler::mouse_button_released[sf::Mouse::ButtonCount];

sf::Vector2f InputHandler::joystick_pos_xy = sf::Vector2f();
float InputHandler::joystick_pos_z = 0.0f;
float InputHandler::joystick_pos_r = 0.0f;
float InputHandler::joystick_xy_delta;
float InputHandler::joystick_axis_pos[sf::Joystick::AxisCount];

InputEventHandler::InputEventHandler()
{
    InputHandler::input_event_handlers.push_back(this);
}

InputEventHandler::~InputEventHandler()
{
}

void InputHandler::initialize()
{
    memset(mouse_button_down, 0, sizeof(mouse_button_down));
    memset(keyboard_button_down, 0, sizeof(keyboard_button_down));
    memset(joystick_axis_pos, 0, sizeof(joystick_axis_pos));
#ifdef __ANDROID__
    touch_screen = true;
#endif
    last_key_press.code = sf::Keyboard::Unknown;
}

void InputHandler::preEventsUpdate()
{
    if (!windowManager)
        windowManager = engine->getObject("windowManager");

    for(unsigned int n=0; n<sf::Keyboard::KeyCount; n++)
    {
        keyboard_button_pressed[n] = false;
        keyboard_button_released[n] = false;
    }
    for(unsigned int n=0; n<sf::Mouse::ButtonCount; n++)
    {
        mouse_button_pressed[n] = false;
        mouse_button_released[n] = false;
    }

    mouse_wheel_delta = 0;
}

void InputHandler::handleEvent(sf::Event& event)
{
    if (event.type == sf::Event::KeyPressed)
    {
        if (event.key.code > sf::Keyboard::Unknown && event.key.code < sf::Keyboard::KeyCount)
        {
            keyboard_button_down[event.key.code] = true;
            keyboard_button_pressed[event.key.code] = true;
        }
        last_key_press = event.key;
    }
	if (event.type == sf::Event::KeyReleased)
	{
        if (event.key.code > sf::Keyboard::Unknown && event.key.code < sf::Keyboard::KeyCount)
        {
            keyboard_button_down[event.key.code] = false;
            keyboard_button_released[event.key.code] = true;
        }
	}
    if (event.type == sf::Event::TextEntered && event.text.unicode > 31 && event.text.unicode < 128)
    {
        if (last_key_press.code != sf::Keyboard::Unknown)
        {
            fireKeyEvent(last_key_press, event.text.unicode);
            last_key_press.code = sf::Keyboard::Unknown;
        }
    }
    if (event.type == sf::Event::MouseWheelMoved)
        mouse_wheel_delta += event.mouseWheel.delta;
    if (event.type == sf::Event::MouseButtonPressed)
    {
        mouse_button_down[event.mouseButton.button] = true;
        mouse_button_pressed[event.mouseButton.button] = true;
    }
    if (event.type == sf::Event::MouseButtonReleased)
    {
        mouse_button_down[event.mouseButton.button] = false;
        mouse_button_released[event.mouseButton.button] = true;
    }
}

void InputHandler::postEventsUpdate()
{
    if (last_key_press.code != sf::Keyboard::Unknown)
    {
        InputHandler::fireKeyEvent(last_key_press, -1);
        last_key_press.code = sf::Keyboard::Unknown;
    }

#ifdef __ANDROID__
    if (sf::Touch::isDown(0))
    {
        mouse_position = realWindowPosToVirtual(sf::Touch::getPosition(0));
        if (!mouse_button_down[sf::Mouse::Left])
            mouse_button_pressed[sf::Mouse::Left] = true;
        mouse_button_down[sf::Mouse::Left] = true;
    }else{
        if (mouse_button_down[sf::Mouse::Left])
            mouse_button_released[sf::Mouse::Left] = true;
        mouse_button_down[sf::Mouse::Left] = false;
    }
#else
    mouse_position = realWindowPosToVirtual(sf::Mouse::getPosition(windowManager->window));
#endif
    mouse_position = mouse_transform.transformPoint(mouse_position);
    
    if (touch_screen)
    {
        bool any_button_down = false;
        for(unsigned int n=0; n<sf::Mouse::ButtonCount; n++)
            if (mouse_button_down[n] || mouse_button_released[n])
                any_button_down = true;
        if (!any_button_down)
        {
            mouse_position = sf::Vector2f(-1, -1);
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
    }
}

void InputHandler::setMousePos(sf::Vector2f position)
{
    if (!windowManager)
        windowManager = engine->getObject("windowManager");

    sf::Mouse::setPosition(virtualWindowPosToReal(position), windowManager->window);
    mouse_position = realWindowPosToVirtual(sf::Mouse::getPosition(windowManager->window));
}

void InputHandler::fireKeyEvent(sf::Event::KeyEvent key, int unicode)
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

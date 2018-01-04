#ifndef INPUT_H
#define INPUT_H

#include "windowManager.h"
#include "stringImproved.h"
#include "Updatable.h"

class InputEventHandler: public virtual PObject
{
public:
    InputEventHandler();
    virtual ~InputEventHandler();
    
    virtual void handleKeyPress(sf::Event::KeyEvent key, int unicode) = 0;
protected:
private:
};

class InputHandler
{
public:
    static bool touch_screen;
    static sf::Transform mouse_transform;

    static PVector<InputEventHandler> input_event_handlers;

    static bool keyboardIsDown(sf::Keyboard::Key key) { return keyboard_button_down[key]; }
    static bool keyboardIsPressed(sf::Keyboard::Key key) { return keyboard_button_pressed[key]; }
    static bool keyboardIsReleased(sf::Keyboard::Key key) { return keyboard_button_released[key]; }

    static sf::Vector2f getMousePos() { return mouse_position; }
    static void setMousePos(sf::Vector2f position);
    static bool mouseIsDown(sf::Mouse::Button button) { return mouse_button_down[button]; }
    static bool mouseIsPressed(sf::Mouse::Button button) { return mouse_button_pressed[button]; }
    static bool mouseIsReleased(sf::Mouse::Button button) { return mouse_button_released[button]; }
    static float getMouseWheelDelta() { return mouse_wheel_delta; }
    
    static sf::Vector2f getJoysticXYPos() { return joystick_pos_xy; }
    static float        getJoysticZPos()  { return joystick_pos_z; }
    static float        getJoysticRPos()  { return joystick_pos_r; }

private:
    static P<WindowManager> windowManager;

    static sf::Vector2f mouse_position;
    static float mouse_wheel_delta;
    static bool keyboard_button_down[sf::Keyboard::KeyCount];
    static bool keyboard_button_pressed[sf::Keyboard::KeyCount];
    static bool keyboard_button_released[sf::Keyboard::KeyCount];
    static sf::Event::KeyEvent last_key_press;

    static bool mouse_button_down[sf::Mouse::ButtonCount];
    static bool mouse_button_pressed[sf::Mouse::ButtonCount];
    static bool mouse_button_released[sf::Mouse::ButtonCount];
    
    static sf::Vector2f joystick_pos_xy;
    static float joystick_pos_z;
    static float joystick_pos_r;
    static float joystick_xy_delta;
    static float joystick_axis_pos[sf::Joystick::AxisCount];

    constexpr static float joystick_xy_hysteresis = 10;
    constexpr static float joystick_z_hysteresis = 3;
    constexpr static float joystick_r_hysteresis = 10;

    static void initialize();
    static void preEventsUpdate();
    static void postEventsUpdate();
    static void handleEvent(sf::Event& event);
    
    static void fireKeyEvent(sf::Event::KeyEvent key, int unicode);
    static sf::Vector2f realWindowPosToVirtual(sf::Vector2i position);
    static sf::Vector2i virtualWindowPosToReal(sf::Vector2f position);

    friend class Engine;
};


#endif//INPUT_H

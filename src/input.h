#ifndef INPUT_H
#define INPUT_H

#include "windowManager.h"
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

class JoystickEventHandler: public virtual PObject
{
public:
    JoystickEventHandler();
    virtual ~JoystickEventHandler();
    
    virtual void handleJoystickAxis(unsigned int joystick, sf::Joystick::Axis axis, float position) = 0;
    virtual void handleJoystickButton(unsigned int joystick, unsigned int button, bool state) = 0;
protected:
private:
};

class InputHandler
{
public:
    static bool touch_screen;
    static sf::Transform mouse_transform;

    static PVector<InputEventHandler> input_event_handlers;
    static PVector<JoystickEventHandler> joystick_event_handlers;

    static bool keyboardIsDown(sf::Keyboard::Key key) { return keyboard_button_down[key]; }
    static bool keyboardIsPressed(sf::Keyboard::Key key) { return keyboard_button_pressed[key]; }
    static bool keyboardIsReleased(sf::Keyboard::Key key) { return !keyboard_button_pressed[key] && keyboard_button_released[key]; }

    static sf::Vector2f getMousePos() { return mouse_position; }
    static void setMousePos(sf::Vector2f position);
    static bool mouseIsDown(sf::Mouse::Button button) { return mouse_button_down[button]; }
    static bool mouseIsPressed(sf::Mouse::Button button) { return mouse_button_pressed[button]; }
    static bool mouseIsReleased(sf::Mouse::Button button) { return !mouse_button_pressed[button] && mouse_button_released[button]; }
    static float getMouseWheelDelta() { return mouse_wheel_delta; }
    
    static sf::Vector2f getJoysticXYPos() { return sf::Vector2f(joystick_axis_pos[0][sf::Joystick::X], joystick_axis_pos[0][sf::Joystick::Y]); }
    static float        getJoysticZPos()  { return joystick_axis_pos[0][sf::Joystick::Z]; }
    static float        getJoysticRPos()  { return joystick_axis_pos[0][sf::Joystick::R]; }

    static float getJoysticAxisPos(unsigned int joystickId, sf::Joystick::Axis axis){ return joystick_axis_pos[joystickId][axis];}
    static bool getJoysticButtonState(unsigned int joystickId, unsigned int button){ return joystick_button_down[joystickId][button];}

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
    
    static float joystick_axis_pos[sf::Joystick::Count][sf::Joystick::AxisCount];
    static float joystick_axis_changed[sf::Joystick::Count][sf::Joystick::AxisCount];
    static bool joystick_button_down[sf::Joystick::Count][sf::Joystick::ButtonCount];
    static bool joystick_button_changed[sf::Joystick::Count][sf::Joystick::ButtonCount];
    constexpr static float joystick_axis_snap_to_0_range = 5;

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

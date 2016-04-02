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
        
        virtual void handleKeyPress(sf::Keyboard::Key key, int unicode);
    protected:
    private:
};

class InputHandler
{
    static P<WindowManager> windowManager;

    static sf::Vector2f mousePos;
    static float mouse_wheel_delta;
    static bool mouse_button_down[sf::Mouse::ButtonCount];
    static bool keyboard_button_down[sf::Keyboard::KeyCount];

    static bool mouseButtonDown[sf::Mouse::ButtonCount];
    static bool mouseButtonPressed[sf::Mouse::ButtonCount];
    static bool mouseButtonReleased[sf::Mouse::ButtonCount];
    static bool keyboardButtonDown[sf::Keyboard::KeyCount];
    static bool keyboardButtonPressed[sf::Keyboard::KeyCount];
    static bool keyboardButtonReleased[sf::Keyboard::KeyCount];
    static bool joystickButtonDown[sf::Joystick::ButtonCount];
    static bool joystickButtonPressed[sf::Joystick::ButtonCount];
    static bool joystickButtonReleased[sf::Joystick::ButtonCount];
    
    static sf::Vector2f joystick_pos_xy;
    static float joystick_pos_z;
    static float joystick_pos_r;
    static float joystick_xy_delta;
    static bool  joystick_button_down[sf::Joystick::ButtonCount];
    static float joystick_axis_pos[sf::Joystick::AxisCount];
    constexpr static float joystick_xy_hysteresis = 10;
    constexpr static float joystick_z_hysteresis = 3;
    constexpr static float joystick_r_hysteresis = 10;

    static void initialize();
    
    static void fireKeyEvent(sf::Keyboard::Key key, int unicode);
    static sf::Vector2f realWindowPosToVirtual(sf::Vector2i position);
    static sf::Vector2i virtualWindowPosToReal(sf::Vector2f position);
public:
    static bool touch_screen;
    static bool joystick;
    static sf::Transform mouse_transform;
    static PVector<InputEventHandler> input_event_handlers;

    static void update();

    static bool keyboardIsDown(sf::Keyboard::Key key) { return keyboardButtonDown[key]; }
    static bool keyboardIsPressed(sf::Keyboard::Key key) { return keyboardButtonPressed[key]; }
    static bool keyboardIsReleased(sf::Keyboard::Key key) { return keyboardButtonReleased[key]; }

    static sf::Vector2f getMousePos() { return mousePos; }
    static void setMousePos(sf::Vector2f position);
    static bool mouseIsDown(sf::Mouse::Button button) { return mouseButtonDown[button]; }
    static bool mouseIsPressed(sf::Mouse::Button button) { return mouseButtonPressed[button]; }
    static bool mouseIsReleased(sf::Mouse::Button button) { return mouseButtonReleased[button]; }
    static float getMouseWheelDelta() { return mouse_wheel_delta; }
    
    static sf::Vector2f getJoysticXYPos() { return joystick_pos_xy; }
    static float        getJoysticZPos()  { return joystick_pos_z; }
    static float        getJoysticRPos()  { return joystick_pos_r; }

    friend class Engine;
};


#endif//INPUT_H

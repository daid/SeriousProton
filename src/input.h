#ifndef INPUT_H
#define INPUT_H

#include "windowManager.h"
#include "Updatable.h"
#include <SDL.h>


class InputEventHandler: public virtual PObject
{
public:
    InputEventHandler();
    virtual ~InputEventHandler();
    
    virtual void handleKeyPress(const SDL_KeyboardEvent& key, int unicode) = 0;
protected:
private:
};

class JoystickEventHandler: public virtual PObject
{
public:
    JoystickEventHandler();
    virtual ~JoystickEventHandler();
    
    virtual void handleJoystickAxis(unsigned int joystick, int axis, float position) = 0;
    virtual void handleJoystickButton(unsigned int joystick, unsigned int button, bool state) = 0;
protected:
private:
};

class InputHandler
{
public:
    static bool touch_screen;

    static PVector<InputEventHandler> input_event_handlers;
    static PVector<JoystickEventHandler> joystick_event_handlers;

    static bool keyboardIsDown(SDL_Keycode key) { return keyboard_button_down[key]; }
    static bool keyboardIsPressed(SDL_Keycode key) { return keyboard_button_pressed[key]; }
    static bool keyboardIsReleased(SDL_Keycode key) { return !keyboard_button_pressed[key] && keyboard_button_released[key]; }

    static float getMouseWheelDelta() { return mouse_wheel_delta; }
    
    static glm::vec2    getJoysticXYPos() { return glm::vec2(joystick_axis_pos[0][0], joystick_axis_pos[0][1]); }
    static float        getJoysticZPos()  { return joystick_axis_pos[0][2]; }
    static float        getJoysticRPos()  { return joystick_axis_pos[0][3]; }

    static float getJoysticAxisPos(unsigned int joystickId, int axis){ return joystick_axis_pos[joystickId][axis];}
    static bool getJoysticButtonState(unsigned int joystickId, unsigned int button){ return joystick_button_down[joystickId][button];}

private:
    static P<Window> window;

    static float mouse_wheel_delta;

    static bool keyboard_button_down[256];
    static bool keyboard_button_pressed[256];
    static bool keyboard_button_released[256];
    static SDL_KeyboardEvent last_key_press;

    static bool mouse_button_down[5];
    static bool mouse_button_pressed[5];
    static bool mouse_button_released[5];
    
    static float joystick_axis_pos[4][4];
    static float joystick_axis_changed[4][4];
    static bool joystick_button_down[4][4];
    static bool joystick_button_changed[4][4];
    constexpr static float joystick_axis_snap_to_0_range = 5;

    static void initialize();
    static void preEventsUpdate();
    static void postEventsUpdate();
    static void handleEvent(const SDL_Event& event);
    
    static void fireKeyEvent(const SDL_KeyboardEvent& key, int unicode);

    friend class Engine;
};


#endif//INPUT_H

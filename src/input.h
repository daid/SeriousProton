#ifndef INPUT_H
#define INPUT_H

#include "windowManager.h"
#include "Updatable.h"
#include <SDL.h>


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

    static PVector<JoystickEventHandler> joystick_event_handlers;

    static glm::vec2    getJoysticXYPos() { return glm::vec2(joystick_axis_pos[0][0], joystick_axis_pos[0][1]); }
    static float        getJoysticZPos()  { return joystick_axis_pos[0][2]; }
    static float        getJoysticRPos()  { return joystick_axis_pos[0][3]; }

    static float getJoysticAxisPos(unsigned int joystickId, int axis){ return joystick_axis_pos[joystickId][axis];}
    static bool getJoysticButtonState(unsigned int joystickId, unsigned int button){ return joystick_button_down[joystickId][button];}

private:
    static P<Window> window;

    static float joystick_axis_pos[4][4];
    static float joystick_axis_changed[4][4];
    static bool joystick_button_down[4][4];
    static bool joystick_button_changed[4][4];
    constexpr static float joystick_axis_snap_to_0_range = 5;

    static void initialize();
    static void preEventsUpdate();
    static void postEventsUpdate();
    static void handleEvent(const SDL_Event& event);
    
    friend class Engine;
};


#endif//INPUT_H

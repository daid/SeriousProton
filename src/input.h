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
    static int mouse_wheel_delta;
    static bool mouse_button_down[sf::Mouse::ButtonCount];
    static bool keyboard_button_down[sf::Keyboard::KeyCount];

    static bool mouseButtonDown[sf::Mouse::ButtonCount];
    static bool mouseButtonPressed[sf::Mouse::ButtonCount];
    static bool mouseButtonReleased[sf::Mouse::ButtonCount];
    static bool keyboardButtonDown[sf::Keyboard::KeyCount];
    static bool keyboardButtonPressed[sf::Keyboard::KeyCount];
    static bool keyboardButtonReleased[sf::Keyboard::KeyCount];

    static void initialize();
    
    static void fireKeyEvent(sf::Keyboard::Key key, int unicode);
public:
    static bool touch_screen;
    static sf::Transform mouse_transform;
    static PVector<InputEventHandler> input_event_handlers;

    static void update();

    static bool keyboardIsDown(sf::Keyboard::Key key) { return keyboardButtonDown[key]; }
    static bool keyboardIsPressed(sf::Keyboard::Key key) { return keyboardButtonPressed[key]; }
    static bool keyboardIsReleased(sf::Keyboard::Key key) { return keyboardButtonReleased[key]; }

    static sf::Vector2f getMousePos() { return mousePos; }
    static bool mouseIsDown(sf::Mouse::Button button) { return mouseButtonDown[button]; }
    static bool mouseIsPressed(sf::Mouse::Button button) { return mouseButtonPressed[button]; }
    static bool mouseIsReleased(sf::Mouse::Button button) { return mouseButtonReleased[button]; }
    static int getMouseWheelDelta() { return mouse_wheel_delta; }

    friend class Engine;
};


#endif//INPUT_H

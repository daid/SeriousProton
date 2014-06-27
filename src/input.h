#ifndef INPUT_H
#define INPUT_H

#include "windowManager.h"
#include "Updatable.h"

class InputHandler : public virtual PObject
{
    static P<WindowManager> windowManager;

    static sf::Vector2f mousePos;
    static int mouse_wheel_delta;
    static bool mouse_button_down[sf::Mouse::ButtonCount];
    static bool keyboard_button_down[sf::Keyboard::KeyCount];

    static bool mouseButtonDown[sf::Mouse::ButtonCount];
    static bool mouseButtonPressed[sf::Mouse::ButtonCount];
    static bool keyboardButtonDown[sf::Keyboard::KeyCount];
    static bool keyboardButtonPressed[sf::Keyboard::KeyCount];

    static void initialize();
public:
    static void update();

    static bool keyboardIsDown(sf::Keyboard::Key key) { return keyboardButtonDown[key]; }
    static bool keyboardIsPressed(sf::Keyboard::Key key) { return keyboardButtonPressed[key]; }

    static sf::Vector2f getMousePos() { return mousePos; }
    static bool mouseIsDown(sf::Mouse::Button button) { return mouseButtonDown[button]; }
    static bool mouseIsPressed(sf::Mouse::Button button) { return mouseButtonPressed[button]; }
    static int getMouseWheelDelta() { return mouse_wheel_delta; }

    friend class Engine;
};


#endif//INPUT_H

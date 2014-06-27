#ifndef INPUT_H
#define INPUT_H

#include "windowManager.h"
#include "Updatable.h"

class InputHandler
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

class PlayerController: public virtual PObject
{
    int index;
public:
    static const int buttonCount = 6;
    sf::Keyboard::Key keyBind[4 + buttonCount];

    PlayerController(int nr)
    : index(nr)
    {
        switch(nr)
        {
        case 0:
        default:
            keyBind[0] = sf::Keyboard::Left;
            keyBind[1] = sf::Keyboard::Right;
            keyBind[2] = sf::Keyboard::Up;
            keyBind[3] = sf::Keyboard::Down;
            keyBind[4] = sf::Keyboard::Space;
            keyBind[5] = sf::Keyboard::Z;
            keyBind[6] = sf::Keyboard::X;
            keyBind[7] = sf::Keyboard::C;
            keyBind[8] = sf::Keyboard::V;
            keyBind[9] = sf::Keyboard::B;
            break;
        case 1:
            keyBind[0] = sf::Keyboard::A;
            keyBind[1] = sf::Keyboard::D;
            keyBind[2] = sf::Keyboard::W;
            keyBind[3] = sf::Keyboard::S;
            keyBind[4] = sf::Keyboard::Q;
            keyBind[5] = sf::Keyboard::E;
            keyBind[6] = sf::Keyboard::R;
            keyBind[7] = sf::Keyboard::F;
            keyBind[8] = sf::Keyboard::T;
            keyBind[9] = sf::Keyboard::G;
            break;
        }
    }
    virtual ~PlayerController() {}

    bool left() { return sf::Keyboard::isKeyPressed(keyBind[0]); }
    bool right() { return sf::Keyboard::isKeyPressed(keyBind[1]); }
    bool up() { return sf::Keyboard::isKeyPressed(keyBind[2]); }
    bool down() { return sf::Keyboard::isKeyPressed(keyBind[3]); }
    bool button(int idx) { return sf::Keyboard::isKeyPressed(keyBind[4 + idx]); }
};


#endif//INPUT_H

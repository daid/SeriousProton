#ifndef INPUT_H
#define INPUT_H

#include "windowManager.h"
#include "Updatable.h"

class InputHandler : public Updatable
{
    P<WindowManager> windowManager;

    sf::Vector2f mousePos;
    bool mouseButtonDown[sf::Mouse::ButtonCount];
    bool mouseButtonPressed[sf::Mouse::ButtonCount];
public:
    virtual ~InputHandler() {}
    virtual void update(float delta);
    
    sf::Vector2f getMousePos() { return mousePos; }
    bool mouseIsDown(int button) { return mouseButtonDown[button]; }
    bool mouseIsPressed(int button) { return mouseButtonPressed[button]; }
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

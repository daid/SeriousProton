#ifndef PLAYERCONTROLLER_H
#define PLAYERCONTROLLER_H

#include "Updatable.h"
#include "windowManager.h"
class PlayerController: public virtual PObject
{
public:
    static const int buttonCount = 6;
    sf::Keyboard::Key keyBind[4 + buttonCount];

    PlayerController(int nr);
    virtual ~PlayerController() {}

    bool left() { return sf::Keyboard::isKeyPressed(keyBind[0]); }
    bool right() { return sf::Keyboard::isKeyPressed(keyBind[1]); }
    bool up() { return sf::Keyboard::isKeyPressed(keyBind[2]); }
    bool down() { return sf::Keyboard::isKeyPressed(keyBind[3]); }
    bool button(int idx) { return sf::Keyboard::isKeyPressed(keyBind[4 + idx]); }
};

#endif // PLAYERCONTROLLER_H

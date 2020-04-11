#include "PlayerController.h"

PlayerController::PlayerController(int nr)
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

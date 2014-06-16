#include "input.h"
#include "engine.h"

void InputHandler::update(float delta)
{
    if (!windowManager)
        windowManager = engine->getObject("windowManager");

    mousePos = sf::Vector2f(sf::Mouse::getPosition() - windowManager->window.getPosition());
    mousePos.x *= float(windowManager->virtualSize.x) / float(windowManager->window.getSize().x);
    mousePos.y *= float(windowManager->virtualSize.y) / float(windowManager->window.getSize().y);
    for(unsigned int n=0; n<sf::Mouse::ButtonCount; n++)
    {
        bool down = sf::Mouse::isButtonPressed(sf::Mouse::Button(n)) && windowManager->hasFocus();
        mouseButtonPressed[n] = (!mouseButtonDown[n] && down);
        mouseButtonDown[n] = down;
    }
}

#include "tween.h"

template<> sf::Color Tween<sf::Color>::tweenApply(float f, const sf::Color& value0, const sf::Color& value1)
{
    return sf::Color(
        static_cast<sf::Uint8>(value0.r + (value1.r - value0.r) * f),
        static_cast<sf::Uint8>(value0.g + (value1.g - value0.g) * f),
        static_cast<sf::Uint8>(value0.b + (value1.b - value0.b) * f),
        static_cast<sf::Uint8>(value0.a + (value1.a - value0.a) * f)
    );
}

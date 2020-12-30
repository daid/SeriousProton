#ifndef TWEEN_H
#define TWEEN_H

#include <SFML/Graphics/Color.hpp>
/**
Tweening functions. Allows for none-linear effects and stuff.
 */

template<typename T> class Tween
{
protected:
    static T tweenApply(float f, const T& value0, const T& value1);
public:
    static inline T linear(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        float t = (time_now - time_start) / (time_end - time_start);
        return tweenApply(t, value0, value1);
    }
    
    static inline T easeInQuad(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        float t = (time_now - time_start) / (time_end - time_start);
        return tweenApply(t * t, value0, value1);
    }
    static inline T easeOutQuad(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        float t = (time_now - time_start) / (time_end - time_start);
        return tweenApply(-t * (t - 2.0f), value0, value1);
    }
    static inline T easeInCubic(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        float t = (time_now - time_start) / (time_end - time_start);
        return tweenApply(t * t * t, value0, value1);
    }
    static inline T easeOutCubic(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {//BUGGED!
        float t = (time_now - time_start) / (time_end - time_start);
        t -= 1.0;
        return tweenApply(-(t * t * t + 1), value0, value1);
    }
};

template<typename T>
T Tween<T>::tweenApply(float f, const T& value0, const T& value1)
{
    return value0 + (value1 - value0) * f;
}

template<> sf::Color Tween<sf::Color>::tweenApply(float f, const sf::Color& value0, const sf::Color& value1);

#endif//TWEEN_H

#ifndef TWEEN_H
#define TWEEN_H

/**
Tweening functions. Allows for none-linear effects and stuff.
 */

template<typename T> class Tween
{
public:
    static inline T easeInQuad(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        float t = (time_now - time_start) / (time_end - time_start);
        return value0 + (value1 - value0) * t * t;
    }
    static inline T easeOutQuad(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        float t = (time_now - time_start) / (time_end - time_start);
        return value0 - (value1 - value0) * t * (t - 2.0f);
    }
};

#endif//TWEEN_H

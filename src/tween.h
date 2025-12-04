#pragma once

#include <glm/gtc/type_precision.hpp>
#include <algorithm>

// Tweening functions, for non-linear animations and effects.

template<typename T> class Tween
{
protected:
    static constexpr float PI = 3.14159265358979323846f;
    static constexpr float PI_2 = 1.57079632679489661923f;
    static T tweenApply(float f, const T& value0, const T& value1);

    static inline float normalizeTime(float time_now, float time_start, float time_end)
    {
        return std::clamp((time_now - time_start) / (time_end - time_start), 0.0f, 1.0f);
    }

public:
    // Linear interpolation
    static inline T linear(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        return tweenApply(t, value0, value1);
    }

    // Ease in: J-shaped (slow start, fast end)
    // Ease out: r-shaped (fast start, slow end; inversion of ease in)
    // Ease in-out: S-shaped (slow start, fast middle, slow end)
    // Adapted from warrenm/AHEasing (public domain)

    // Sine, cubic, quart, quint, and expo curves each lengthen the in and/or
    // out segment of the curve with increasingly fast transitions
    // https://easings.net/#easeInSine
    static inline T easeInSine(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        return tweenApply(1.0f - cosf(t * PI_2), value0, value1);
    }
    // https://easings.net/#easeOutSine
    static inline T easeOutSine(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        return tweenApply(sinf((t * PI_2)), value0, value1);
    }
    // https://easings.net/#easeInOutSine
    static inline T easeInOutSine(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        return tweenApply(0.5f * (1.0f - cosf(t * PI)), value0, value1);
    }

    // Quadratic
    // https://easings.net/#easeInQuad
    static inline T easeInQuad(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        return tweenApply(t * t, value0, value1);
    }
    // https://easings.net/#easeOutQuad
    static inline T easeOutQuad(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        return tweenApply(-(t * (t - 2.0f)), value0, value1);
    }
    // https://easings.net/#easeInOutQuad
    static inline T easeInOutQuad(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);

        if (t < 0.5f)
            return tweenApply(2.0f * t * t, value0, value1);

        return tweenApply((-2.0f * t * t) + (4.0f * t) - 1.0f, value0, value1);
    }

    // https://easings.net/#easeInCubic
    static inline T easeInCubic(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        return tweenApply(t * t * t, value0, value1);
    }
    // https://easings.net/#easeOutCubic
    static inline T easeOutCubic(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end) - 1.0f;
        return tweenApply((t * t * t) + 1.0f, value0, value1);
    }
    // https://easings.net/#easeInOutCubic
    static inline T easeInOutCubic(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        float t = normalizeTime(time_now, time_start, time_end);

        if (t < 0.5f)
            return tweenApply(4.0f * (t * t * t), value0, value1);

        t = (t * 2.0f) - 2.0f;
        return tweenApply(0.5f * (t * t * t) + 1.0f, value0, value1);
    }

    // https://easings.net/#easeInQuartic
    static inline T easeInQuartic(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        return tweenApply(t * t * t * t, value0, value1);
    }
    // https://easings.net/#easeOutQuartic
    static inline T easeOutQuartic(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end) - 1.0f;
        return tweenApply(1.0f - (t * t * t * t), value0, value1);
    }
    // https://easings.net/#easeInOutQuartic
    static inline T easeInOutQuartic(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        float t = normalizeTime(time_now, time_start, time_end);

        if (t < 0.5f)
            return tweenApply(8.0f * (t * t * t * t), value0, value1);

        t -= 1.0f;
        return tweenApply(-8.0f * (t * t * t * t) + 1.0f, value0, value1);
    }

    // https://easings.net/#easeInQuintic
    static inline T easeInQuintic(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        return tweenApply(t * t * t * t * t, value0, value1);
    }
    // https://easings.net/#easeOutQuintic
    static inline T easeOutQuintic(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end) - 1.0f;
        return tweenApply((t * t * t * t * t) + 1.0f, value0, value1);
    }
    // https://easings.net/#easeInOutQuintic
    static inline T easeInOutQuintic(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        float t = normalizeTime(time_now, time_start, time_end);

        if (t < 0.5f)
            return tweenApply(16.0f * (t * t * t * t * t), value0, value1);

        t = (t * 2.0f) - 2.0f;
        return tweenApply(0.5f * (t * t * t * t * t) + 1.0f, value0, value1);
    }

    // https://easings.net/#easeInExponential
    static inline T easeInExponential(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        if (t == 0.0f) return value0;

        return tweenApply(powf(2.0f, 10.0f * (t - 1.0f)), value0, value1);
    }
    // https://easings.net/#easeOutExponential
    static inline T easeOutExponential(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        if (t == 1.0f) return value1;

        return tweenApply(1.0f - powf(2.0f, -10.0f * t), value0, value1);
    }
    // https://easings.net/#easeInOutExponential
    static inline T easeInOutExponential(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        if (t == 0.0f) return value0;
        if (t == 1.0f) return value1;

        if (t < 0.5f)
            return tweenApply(0.5f * powf(2.0f, (20.0f * t) - 10.0f), value0, value1);

        return tweenApply(0.5f * (2.0f - powf(2.0f, (-20.0f * t) + 10.0f)), value0, value1);
    }

    // More gradual curve resembling a quarter oval
    // https://easings.net/#easeInCircular
    static inline T easeInCircular(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        return tweenApply(1.0f - sqrtf(1.0f - (t * t)), value0, value1);
    }
    // https://easings.net/#easeOutCircular
    static inline T easeOutCircular(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end) - 1.0f;
        return tweenApply(sqrtf(1.0f - (t * t)), value0, value1);
    }
    // https://easings.net/#easeInOutCircular
    static inline T easeInOutCircular(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        if (t < 0.5f)
            return tweenApply(0.5f * (1.0f - sqrtf(1.0f - 4.0f * (t * t))), value0, value1);

        return tweenApply(0.5f * (sqrtf(-((2.0f * t) - 3.0f) * ((2.0f * t) - 1.0f)) + 1.0f), value0, value1);
    }

    // These functions intentionally overshoot the 0-1 range.

    // Damped sine wave resembling a bounce
    // https://easings.net/#easeInElastic
    static inline T easeInElastic(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        return tweenApply(-powf(2.0f, 10.0f * t - 10.0f) * sinf((t * 10.0f - 10.75f) * (PI * 2.0f) / 3.0f), value0, value1);
    }
    // https://easings.net/#easeOutElastic
    static inline T easeOutElastic(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        if (t == 0.0f)
            return tweenApply(0.0f, value0, value1);
        if (t == 1.0f)
            return tweenApply(1.0f, value0, value1);

        return tweenApply(powf(2.0f, -10.0f * t) * sinf((t * 10.0f - 0.75f) * (PI * 2.0f) / 3.0f) + 1.0f, value0, value1);
    }
    // https://easings.net/#easeInOutElastic
    static inline T easeInOutElastic(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        const float elasticity = (PI * 2.0f) / 4.5f;

        if (t < 0.5f)
            return tweenApply(-(powf(2.0f, 20.0f * t - 10.0f) * sinf((20.0f * t - 11.125f) * elasticity)) / 2.0f, value0, value1);

        return tweenApply((powf(2.0f, -20.0f * t + 10.0f) * sinf((20.0f * t - 11.125f) * elasticity)) / 2.0f + 1.0f, value0, value1);
    }

    // Overshooting version of cubic
    // https://easings.net/#easeInBack
    static inline T easeInBack(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        return tweenApply((t * t * t) - t * sinf(t * PI), value0, value1);
    }
    // https://easings.net/#easeOutBack
    static inline T easeOutBack(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = 1.0f - normalizeTime(time_now, time_start, time_end);
        return tweenApply(1.0f - ((t * t * t) - t * sinf(t * PI)), value0, value1);
    }
    // https://easings.net/#easeInOutBack
    static inline T easeInOutBack(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        float f = 2.0f * t;

        if (t < 0.5f)
            return tweenApply(0.5f * ((f * f * f) - (f * sinf(f * PI))), value0, value1);

        f = 1.0f - (f - 1.0f);
        return tweenApply(0.5f * (1.0f - ((f * f * f) - (f * sinf(f * PI)))) + 0.5f, value0, value1);
    }
};

template<typename T>
T Tween<T>::tweenApply(float f, const T& value0, const T& value1)
{
    return value0 + (value1 - value0) * f;
}

template<> glm::u8vec4 Tween<glm::u8vec4>::tweenApply(float f, const glm::u8vec4& value0, const glm::u8vec4& value1);

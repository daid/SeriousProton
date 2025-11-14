#pragma once

#include <glm/gtc/type_precision.hpp>
#include <algorithm>
#include <cmath>

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

    // Ease in: J-shaped (slow start, fast end)   Ease out: r-shaped (fast start, slow end; inversion of ease in)
    // Ease in-out: S-shaped (slow start, fast middle, slow end)
    // Adapted from warrenm/AHEasing (public domain)
    static inline T easeInSine(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        return tweenApply(sin((t - 1.0f) * PI_2), value0, value1);
    }
    static inline T easeOutSine(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        return tweenApply(sin((t * PI_2)), value0, value1);
    }
    static inline T easeInOutSine(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        return tweenApply(0.5f * (1.0f - cos(t * PI)), value0, value1);
    }

    // Quadratic
    static inline T easeInQuad(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        return tweenApply(t * t, value0, value1);
    }
    static inline T easeOutQuad(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = 1.0f - normalizeTime(time_now, time_start, time_end);
        return tweenApply(-(t * (t - 2.0f)), value0, value1);
    }
    static inline T easeInOutQuad(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);

        if (t < 0.5f)
            return tweenApply(2.0f * t * t, value0, value1);

        return tweenApply((-2.0f * t * t) + (4.0f * t) - 1.0f, value0, value1);
    }

    static inline T easeInCubic(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        return tweenApply(t * t * t, value0, value1);
    }
    static inline T easeOutCubic(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end) - 1.0f;
        return tweenApply((t * t * t) + 1.0f, value0, value1);
    }
    static inline T easeInOutCubic(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        float t = normalizeTime(time_now, time_start, time_end);

        if (t < 0.5f)
            return tweenApply(4.0f * (t * t * t), value0, value1);

        t = (t * 2.0f) - 2.0f;
        return tweenApply(0.5f * (t * t * t) + 1.0f, value0, value1);
    }

    static inline T easeInQuartic(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        return tweenApply(t * t * t * t, value0, value1);
    }
    static inline T easeOutQuartic(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end) - 1.0f;
        return tweenApply(((t * t * t) * (1.0f - t)) + 1.0f, value0, value1);
    }
    static inline T easeInOutQuartic(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        float t = normalizeTime(time_now, time_start, time_end);

        if (t < 0.5f)
            return tweenApply(8.0f * (t * t * t * t), value0, value1);

        t -= 1.0f;
        return tweenApply(-8.0f * (t * t * t * t) + 1.0f, value0, value1);
    }

    static inline T easeInQuintic(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        return tweenApply(t * t * t * t * t, value0, value1);
    }
    static inline T easeOutQuintic(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end) - 1.0f;
        return tweenApply((t * t * t * t * t) + 1.0f, value0, value1);
    }
    static inline T easeInOutQuintic(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        float t = normalizeTime(time_now, time_start, time_end);

        if (t < 0.5f)
            return tweenApply(16.0f * (t * t * t * t * t), value0, value1);

        t = (t * 2.0f) - 2.0f;
        return tweenApply(0.5f * (t * t * t * t * t) + 1.0f, value0, value1);
    }

    static inline T easeInCircular(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        // TODO: Approximate to avoid use of inefficient sqrt in loops
        return tweenApply(1.0f - std::sqrtf(1.0f - (t * t)), value0, value1);
    }
    static inline T easeOutCircular(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        float t = normalizeTime(time_now, time_start, time_end) - 1.0f;
        // TODO: Approximate to avoid use of inefficient sqrt in loops
        return tweenApply(std::sqrtf((2.0f - t) * t), value0, value1);
    }
    static inline T easeInOutCircular(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        // TODO: Approximate to avoid use of inefficient sqrt in loops
        if (t < 0.5f)
            return tweenApply(0.5f * (1.0f - std::sqrtf(1.0f - 4.0f * (t * t))), value0, value1);

        return tweenApply(0.5f * (std::sqrtf(-((2.0f * t) - 3.0f) * ((2.0f * t) - 1.0f)) + 1.0f), value0, value1);
    }

    static inline T easeInExponential(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        float t = normalizeTime(time_now, time_start, time_end);
        if (t == 0.0f) return t;

        t = 10.0f * (t - 1.0f);
        return tweenApply(t * t, value0, value1);
    }
    static inline T easeOutExponential(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        float t = normalizeTime(time_now, time_start, time_end) - 1.0f;
        if (t == 1.0f) return t;

        t = -10.0f * t;
        return tweenApply(1.0f - (t * t), value0, value1);
    }
    static inline T easeInOutExponential(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        if (t == 0.0f || t == 1.0f) return t;

        float f = (-20.0f * t) + 10.0f;

        if (t < 0.5f)
        {
            f = (20.0f * t) - 10.0f;
            return tweenApply(0.5f * (t * t), value0, value1);
        }

        return tweenApply(-0.5f * (f * f) + 1.0f, value0, value1);
    }

    // Damped sine wave
    static inline T easeInElastic(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        return tweenApply(sin(13.0f * PI_2 * t) * std::powf(10.0f * (t - 1.0f), 2.0f), value0, value1);
    }
    static inline T easeOutElastic(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        float t = normalizeTime(time_now, time_start, time_end);
        return tweenApply((-13.0f * PI_2 * (t + 1.0f)) * std::powf((-10.0f * t) + 1.0f, 2.0f), value0, value1);
    }
    static inline T easeInOutElastic(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);

        if (t < 0.5f)
            return tweenApply(0.5f * sin(13.0f * PI_2 * (2.0f * t)) * std::powf(10.0f * ((2.0f * t) - 1.0f), 2.0f), value0, value1);

        return tweenApply(0.5f * (sin(-13.0f * PI_2 * ((2.0f * t - 1.0f) + 1.0f)) * std::powf(-10.0f * (2.0f * t - 1.0f) + 2.0f, 2.0f)), value0, value1);
    }

    // Modeled after the overshooting cubic y = x^3-x*sin(x*pi)
    static inline T easeInBack(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);
        return tweenApply((t * t * t) - t * sin(t * PI), value0, value1);
    }
    static inline T easeOutBack(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = 1.0f - normalizeTime(time_now, time_start, time_end);
        return tweenApply(1.0f - ((t * t * t) - t * sin(t * PI)), value0, value1);
    }
    static inline T easeInOutBack(float time_now, float time_start, float time_end, const T& value0, const T& value1)
    {
        const float t = 1.0f - normalizeTime(time_now, time_start, time_end);
        float f = 2.0f * t;

        if (t < 0.5f)
            return tweenApply(0.5f * ((f * f * f) - (f * sin(f * PI))), value0, value1);

        f = 1.0f - (f - 1.0f);
        return tweenApply(0.5f * (1.0f - ((f * f * f) - (f * sin(f * PI)))) + 0.5f, value0, value1);
    }

    /* CSS-style cubic Bezier curve-based easing with two control-point
       coordinates.

       See https://drafts.csswg.org/css-easing/,
       https://probablymarcus.com/blocks/2015/02/26/using-bezier-curves-as-easing-functions.html,
       and https://code.compassfoundation.io/general/mozilla-central/-/blob/d003925309ce4179c2b33102d138548bc4281da7/dom/smil/nsSMILKeySpline.cpp

       The two vec2 values p1 and p2 respectively represent Bezier control
       points that bend a curve drawn from 0,0 to 1,1. This allows for the
       simulation of other easing functions and generation of arbitrary curves.
       p1 and p2 values can exceed -0.0 and 1.0 to create overshoot effects.

       Examples:

       - Cubic ease-in equivalent:
         Tween<float>::easeCubicBezier(time, 1.0f, 0.0f, glm::vec2{0.32f, 0.0f}, glm::vec2{0.67f, 0.0f}, 0.0f, 255.0f);
       - Ease-out equivalent (ease-in values inverted):
         Tween<float>::easeCubicBezier(time, 1.0f, 0.0f, glm::vec2{0.33f, 1.0f}, glm::vec2{0.68f, 1.0f}, 0.0f, 255.0f);
       - Ease-in-out equivalent (ease-in values transposed as p2.x,0 and p1.x,1):
         Tween<float>::easeCubicBezier(time, 1.0f, 0.0f, glm::vec2{0.67f, 0.0f}, glm::vec2{0.33f, 1.0f}, 0.0f, 255.0f);
    */
    static inline T easeCubicBezier(float time_now, float time_start, float time_end, glm::vec2 p1, glm::vec2 p2, const T& value0, const T& value1)
    {
        const float t = normalizeTime(time_now, time_start, time_end);

        // Return early if linear
        if (p1.x == p1.y && p2.x == p2.y && p1.x == 0.0f && p2.x == 1.0f)
            return tweenApply(t, value0, value1);

        // Coefficients for cubic polynomial
        const glm::vec2 a = {
            (3.0f * p1.x) - (3.0f * p2.x) + 1.0f,
            (3.0f * p1.y) - (3.0f * p2.y) + 1.0f
        };
        const glm::vec2 b = {
            (-6.0f * p1.x) + (3.0f * p2.x),
            (-6.0f * p1.y) + (3.0f * p2.y)
        };
        const glm::vec2 c = {
            3.0f * p1.x,
            3.0f * p1.y
        };

        auto sampleCurveX = [&](float s) -> float { return ((a.x * s + b.x) * s + c.x) * s; };
        auto sampleCurveY = [&](float s) -> float { return ((a.y * s + b.y) * s + c.y) * s; };
        auto sampleCurveDerivativeX = [&](float s) -> float { return (3.0f * a.x * s + 2.0f * b.x) * s + c.x; };

        // Solve for s such that sampleCurveX(s) ~= t
        float s = t;
        // Reduce FP precision, otherwise convergence takes too many iterations
        const float epsilon = 0.0000001f;

        // Try Newton's method first: https://en.wikipedia.org/wiki/Newton%27s_method
        // Limit Newton's iterations for speed. 4 is plenty.
        // Try to avoid more expensive bisection if possible, but Newton's can't
        // solve sufficiently shallow slopes.
        for (int i = 0; i < 4; ++i)
        {
            const float x = sampleCurveX(s) - t;
            if (std::fabs(x) < epsilon) break;
            const float d = sampleCurveDerivativeX(s);
            if (std::fabs(d) < epsilon) break;

            s -= x / d;
            if (s < 0.0f)
            {
                s = 0.0f;
                break;
            }
            if (s > 1.0f)
            {
                s = 1.0f;
                break;
            }
        }

        // If Newton's didn't converge, bisect.
        if (std::fabs(sampleCurveX(s) - t) >= epsilon)
        {
            float s0 = 0.0f;
            float s1 = 1.0f;
            int i = 0;
            s = t;

            // 10 bisections is enough for anyone!
            while (s1 - s0 > epsilon && i++ < 10)
            {
                float x = sampleCurveX(s);

                if (x > t) s1 = s;
                else s0 = s;

                s = 0.5f * (s0 + s1);
            }
        }

        return tweenApply(sampleCurveY(s), value0, value1);
    }
};

template<typename T>
T Tween<T>::tweenApply(float f, const T& value0, const T& value1)
{
    return value0 + (value1 - value0) * f;
}

template<> glm::u8vec4 Tween<glm::u8vec4>::tweenApply(float f, const glm::u8vec4& value0, const glm::u8vec4& value1);

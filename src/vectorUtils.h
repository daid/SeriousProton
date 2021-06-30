#ifndef SFML_EXTRA_VECTOR_UTILS_H
#define SFML_EXTRA_VECTOR_UTILS_H

#include <math.h>

#include <glm/geometric.hpp> //for glm::lenght and glm::dot
#include <glm/gtx/norm.hpp>  //for glm::lenght2

static inline glm::vec2 vec2FromAngle(float angle)
{
    float a = glm::radians(angle);
    return glm::vec2(cosf(a), sinf(a));
}

static inline float vec2ToAngle(const glm::vec2& v)
{
    return glm::degrees(atan2(v.y, v.x));
}

static inline glm::vec2 rotateVec2(const glm::vec2& v, const float& angle)
{
    float a = glm::radians(angle);
    return glm::vec2(cosf(a), sinf(a)) * v.x + glm::vec2(-sinf(a), cosf(a)) * v.y;
}

// Return the intersection of the infinite line trough points p0 and p1 and infinite line trough points p2 and p3.
static inline glm::vec2 lineLineIntersection(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3)
{
    float A1 = p1.y - p0.y;
    float B1 = p0.x - p1.x;
    float C1 = A1*p0.x + B1*p0.y;

    float A2 = p3.y - p2.y;
    float B2 = p2.x - p3.x;
    float C2 = A2 * p2.x + B2 * p2.y;

    float det = A1*B2 - A2*B1;
    if (det == 0)
        return p0;
    return glm::vec2((B2*C1 - B1*C2)/det, (A1 * C2 - A2 * C1) / det);
}

/* Return the difference between angle_a and angle_b within a range of -180 and 180 degrees */
static inline float angleDifference(float angle_a, float angle_b)
{
    float ret = (angle_b - angle_a);
    while(ret > 180) ret -= 360;
    while(ret < -180) ret += 360;
    return ret;
}

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Color.hpp>

/** math.h no longer defines M_PI in C++11. For... reasons? */
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

/**
    SFML is missing a few useful operators on the 2D vectors.
    These are the missing operators.
*/

namespace sf
{
    template <typename T>
    Vector2<T> vector2FromAngle(const T& angle)
    {
        T a = angle / 180.0 * M_PI;
        return Vector2<T>(cosf(a), sinf(a));
    }

    template <typename T>
    T vector2ToAngle(const Vector2<T>& v)
    {
        return atan2(v.y, v.x) / M_PI * 180;
    }

    template <typename T>
    Vector2<T> rotateVector(const Vector2<T>& v, const T& angle)
    {
        T a = angle / 180.0f * M_PI;
        return Vector2<T>(cosf(a), sinf(a)) * v.x + Vector2<T>(-sinf(a), cosf(a)) * v.y;
    }

    template <typename T>
    T length(const Vector2<T>& v)
    {
        return sqrtf(v.x*v.x+v.y*v.y);
    }

    template <typename T>
    Vector2<T> normalize(const Vector2<T>& v)
    {
        return v / length(v);
    }
}

#endif//SFML_EXTRA_VECTOR_UTILS_H

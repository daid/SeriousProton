#ifndef SFML_EXTRA_VECTOR_UTILS_H
#define SFML_EXTRA_VECTOR_UTILS_H

#include <math.h>
#include <SFML/System.hpp>
/**
    SFML is missing a few useful operators on the 2D vectors.
    These are the missing operators.
*/

namespace sf
{
    /** < and > operators are length compares. Return true or false if the distance is longer/shorter then asked distance. */
    template <typename T>
    static inline bool operator > (const Vector2<T>& v, const T& f)
    {
        return (v.x * v.x + v.y * v.y) > f * f;
    }

    template <typename T>
    static inline bool operator < (const Vector2<T>& v, const T& f)
    {
        return (v.x * v.x + v.y * v.y) < f * f;
    }

    template <typename T>
    static inline bool operator >= (const Vector2<T>& v, const T& f)
    {
        return (v.x * v.x + v.y * v.y) >= f * f;
    }

    template <typename T>
    static inline bool operator <= (const Vector2<T>& v, const T& f)
    {
        return (v.x * v.x + v.y * v.y) <= f * f;
    }

    template <typename T>
    static inline bool operator > (const Vector2<T>& v, const Vector2<T>& v2)
    {
        return (v.x * v.x + v.y * v.y) > (v2.x * v2.x + v2.y * v2.y);
    }

    template <typename T>
    static inline bool operator < (const Vector2<T>& v, const Vector2<T>& v2)
    {
        return (v.x * v.x + v.y * v.y) < (v2.x * v2.x + v2.y * v2.y);
    }

    template <typename T>
    static inline bool operator >= (const Vector2<T>& v, const Vector2<T>& v2)
    {
        return (v.x * v.x + v.y * v.y) >= (v2.x * v2.x + v2.y * v2.y);
    }

    template <typename T>
    static inline bool operator <= (const Vector2<T>& v, const Vector2<T>& v2)
    {
        return (v.x * v.x + v.y * v.y) <= (v2.x * v2.x + v2.y * v2.y);
    }

    template <typename T>
    Vector2<T> vector2FromAngle(const T& angle)
    {
        return Vector2<T>(sinf(angle / 180.0 * M_PI), -cosf(angle / 180.0 * M_PI));
    }

    template <typename T>
    T vector2ToAngle(const Vector2<T>& v)
    {
        return atan2(v.x, -v.y) / M_PI * 180;
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
    
    template <typename T>
    T dot(const Vector2<T>& v0, const Vector2<T>& v1)
    {
        return v0.x * v1.x + v0.y * v1.y;
    }

    //Check if C is left of the infinite line from A to B
    template <typename T>
    bool isLeft(const Vector2<T>& a, const Vector2<T>& b, const Vector2<T>& c)
    {
        return ((b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x)) < 0;
    }

	// Return the intersection of the infinite line trough points p0 and p1 and infinite line trough points p2 and p3.
	template <typename T>
    Vector2<T> lineLineIntersection(const Vector2<T>& p0, const Vector2<T>& p1, const Vector2<T>& p2, const Vector2<T>& p3)
    {
        T A1 = p1.y - p0.y;
        T B1 = p0.x - p1.x;
        T C1 = A1*p0.x + B1*p0.y;

        T A2 = p3.y - p2.y;
        T B2 = p2.x - p3.x;
        T C2 = A2 * p2.x + B2 * p2.y;

        T det = A1*B2 - A2*B1;
        if (det == 0)
            return p0;
        return Vector2<T>((B2*C1 - B1*C2)/det, (A1 * C2 - A2 * C1) / det);
    }
}

namespace sf
{
    /** < and > operators are length compares. Return true or false if the distance is longer/shorter then asked distance. */
    template <typename T>
    static inline bool operator > (const Vector3<T>& v, const T& f)
    {
        return (v.x * v.x + v.y * v.y + v.z * v.z) > f * f;
    }

    template <typename T>
    static inline bool operator < (const Vector3<T>& v, const T& f)
    {
        return (v.x * v.x + v.y * v.y + v.z * v.z) < f * f;
    }

    template <typename T>
    static inline bool operator >= (const Vector3<T>& v, const T& f)
    {
        return (v.x * v.x + v.y * v.y + v.z * v.z) >= f * f;
    }

    template <typename T>
    static inline bool operator <= (const Vector3<T>& v, const T& f)
    {
        return (v.x * v.x + v.y * v.y + v.z * v.z) <= f * f;
    }

    template <typename T>
    static inline bool operator > (const Vector3<T>& v, const Vector3<T>& v2)
    {
        return (v.x * v.x + v.y * v.y + v.z * v.z) > (v2.x * v2.x + v2.y * v2.y);
    }

    template <typename T>
    static inline bool operator < (const Vector3<T>& v, const Vector3<T>& v2)
    {
        return (v.x * v.x + v.y * v.y + v.z * v.z) < (v2.x * v2.x + v2.y * v2.y + v2.z * v2.z);
    }

    template <typename T>
    static inline bool operator >= (const Vector3<T>& v, const Vector3<T>& v2)
    {
        return (v.x * v.x + v.y * v.y + v.z * v.z) >= (v2.x * v2.x + v2.y * v2.y + v2.z * v2.z);
    }

    template <typename T>
    static inline bool operator <= (const Vector3<T>& v, const Vector3<T>& v2)
    {
        return (v.x * v.x + v.y * v.y + v.z * v.z) <= (v2.x * v2.x + v2.y * v2.y + v2.z * v2.z);
    }

    template <typename T>
    T length(const Vector3<T>& v)
    {
        return sqrtf(v.x*v.x+v.y*v.y+v.z*v.z);
    }

    template <typename T>
    Vector3<T> normalize(const Vector3<T>& v)
    {
        return v / length(v);
    }
    
    template <typename T>
    T dot(const Vector3<T>& v0, const Vector3<T>& v1)
    {
        return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
    }
}
#endif//SFML_EXTRA_VECTOR_UTILS_H

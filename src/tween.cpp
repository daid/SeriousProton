#include "tween.h"

#include <cstdint>

template<> glm::u8vec4 Tween<glm::u8vec4>::tweenApply(float f, const glm::u8vec4& value0, const glm::u8vec4& value1)
{
    return glm::u8vec4(
        static_cast<uint8_t>(value0.r + (value1.r - value0.r) * f),
        static_cast<uint8_t>(value0.g + (value1.g - value0.g) * f),
        static_cast<uint8_t>(value0.b + (value1.b - value0.b) * f),
        static_cast<uint8_t>(value0.a + (value1.a - value0.a) * f)
    );
}

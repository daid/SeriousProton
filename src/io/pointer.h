#ifndef SP_IO_POINTER_H
#define SP_IO_POINTER_H

#include <cstdint>

namespace sp {
namespace io {

class Pointer
{
public:
    using ID = int64_t;
    static constexpr ID mouse = -1;
    enum class Button
    {
        Touch,
        Left,
        Middle,
        Right,
        X1,
        X2,
        Unknown
    };
private:
};

}//namespace io
}//namespace sp

#endif//SP2_IO_POINTER_H

#ifndef SP2_NON_COPYABLE_H
#define SP2_NON_COPYABLE_H

namespace sp {

/** Baseclass for objects that are not copyable.

    By default, C++ classes can be copied implicitly.
    Any class that holds a unique resource or is a unique resource cannot be copied by nature.
    To ensure copying is properly disable, use this as a base class.
    
    Usage example:
    \code
    class UniqueResource : sp::NonCopyable
    {
    public:
        ...
    };
    \endcode
 */
class NonCopyable
{
protected:
    NonCopyable() = default;
private:
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable(NonCopyable&&) = delete;
    void operator=(const NonCopyable&) = delete;
    void operator=(NonCopyable&&) = delete;
};

}//namespace sp

#endif//SP2_NON_COPYABLE_H

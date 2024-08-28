#ifndef SP2_IO_NETWORK_SOCKET_BASE_H
#define SP2_IO_NETWORK_SOCKET_BASE_H

#include <cstdint>
#include "nonCopyable.h"

#include <stringImproved.h>
extern string steam_debug_status;
extern string steam_debug_log;

namespace sp {
namespace io {
namespace network {


class SocketBase : sp::NonCopyable
{
public:
    void setBlocking(bool blocking);
    void setTimeout(int milliseconds);

protected:
    bool isLastErrorNonBlocking();

#ifdef _WIN32
    uintptr_t handle = (~0);
#else
    intptr_t handle = -1;
#endif
    bool blocking = true;
    
    static void initSocketLib();
    friend class Address;
    friend class Selector;
};

}//namespace network
}//namespace io
}//namespace sp

#endif//SP2_IO_NETWORK_SOCKET_BASE_H

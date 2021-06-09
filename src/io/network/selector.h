#ifndef SP2_IO_NETWORK_SELECTOR_H
#define SP2_IO_NETWORK_SELECTOR_H

#include <io/network/socketBase.h>

namespace sp {
namespace io {
namespace network {


class Selector
{
public:
    Selector();
    Selector(const Selector& other);
    ~Selector();

    Selector& operator =(const Selector& other);
    
    void add(SocketBase& socket);
    void remove(SocketBase& socket);
    void wait(int timeout_ms);
    bool isReady(SocketBase& socket);

private:
    class SelectorData;
    
    SelectorData* data;
};

}//namespace network
}//namespace io
}//namespace sp

#endif//SP2_IO_NETWORK_SELECTOR_H

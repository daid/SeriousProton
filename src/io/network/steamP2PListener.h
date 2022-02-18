#ifndef SP2_IO_NETWORK_STEAM_LISTENER_H
#define SP2_IO_NETWORK_STEAM_LISTENER_H

#include <cstdint>
#include "nonCopyable.h"
#include <memory>


namespace sp {
namespace io {
namespace network {


class SteamP2PSocket;
class SteamP2PListener : sp::NonCopyable
{
public:
    SteamP2PListener();
    ~SteamP2PListener();

    bool listen();
    void close();
    
    bool isListening();

    std::unique_ptr<SteamP2PSocket> accept();

private:
    uint32_t handle = 0;
};

}//namespace network
}//namespace io
}//namespace sp

#endif//SP2_IO_NETWORK_STEAM_LISTENER_H

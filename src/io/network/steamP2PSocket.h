#ifndef SP2_IO_NETWORK_STEAM_P2P_SOCKET_H
#define SP2_IO_NETWORK_STEAM_P2P_SOCKET_H

#include <io/network/streamSocket.h>


namespace sp {
namespace io {
namespace network {


class SteamP2PSocket : public StreamSocket
{
public:
    SteamP2PSocket();
    ~SteamP2PSocket();

    bool connect(uint64_t steam_id);
    virtual void close() override;

    virtual State getState() override;

protected:
    virtual size_t _send(const void* data, size_t size) override;
    virtual size_t _receive(void* data, size_t size) override;

private:
    uint32_t handle = 0;
    std::string recv_buffer;

    friend class SteamP2PListener;
};

}//namespace network
}//namespace io
}//namespace sp

#endif//SP2_IO_NETWORK_STEAM_P2P_SOCKET_H

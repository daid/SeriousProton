#ifndef SP2_IO_NETWORK_UDP_SOCKET_H
#define SP2_IO_NETWORK_UDP_SOCKET_H

#include <stringImproved.h>
#include <io/network/address.h>
#include <io/network/socketBase.h>
#include <io/dataBuffer.h>


namespace sp {
namespace io {
namespace network {

enum class UdpMulticastMode {
    Default, // Default SP multicast; 239.192.hh.ll or FF08::hhll
    SACN,    // sACN multicast; 239.255.hh.ll or FF18::8300:hhll
};

class UdpSocket : public SocketBase
{
public:
    UdpSocket();
    UdpSocket(UdpSocket&& socket);
    ~UdpSocket();

    bool bind(int port);
    bool joinMulticast(int group_nr);
    void close();

    bool send(const void* data, size_t size, const Address& address, int port);
    size_t receive(void* data, size_t size, Address& address, int& port);

    bool send(const DataBuffer& buffer, const Address& address, int port);
    bool receive(DataBuffer& buffer, Address& address, int& port);

    bool sendMulticast(const void* data, size_t size, int group_nr, int port, UdpMulticastMode mode = UdpMulticastMode::Default);
    bool sendMulticast(const DataBuffer& buffer, int group_nr, int port, UdpMulticastMode mode = UdpMulticastMode::Default);

    bool sendBroadcast(const void* data, size_t size, int port);
    bool sendBroadcast(const DataBuffer& buffer, int port);
private:
    bool createSocket();

    bool socket_is_ipv6;
};

}//namespace network
}//namespace io
}//namespace sp

#endif//SP2_IO_NETWORK_TCP_SOCKET_H

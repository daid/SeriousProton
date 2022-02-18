#ifndef SP2_IO_NETWORK_TCP_SOCKET_H
#define SP2_IO_NETWORK_TCP_SOCKET_H

#include <io/network/address.h>
#include <io/network/socketBase.h>
#include <io/network/streamSocket.h>
#include <io/dataBuffer.h>


namespace sp {
namespace io {
namespace network {


class TcpSocket : public SocketBase, public StreamSocket
{
public:
    TcpSocket();
    ~TcpSocket();

    bool connect(const Address& host, int port);
    bool connectSSL(const Address& host, int port);
    void setDelay(bool delay); //Enable of disable the NO_DELAY/Nagle algorithm, allowing for less latency at the cost of more packets.
    virtual void close() override;

    virtual State getState() override;

protected:
    virtual size_t _send(const void* data, size_t size) override;
    virtual size_t _receive(void* data, size_t size) override;

private:
    void* ssl_handle;
    bool connecting = false;

    friend class TcpListener;
};

}//namespace network
}//namespace io
}//namespace sp

#endif//SP2_IO_NETWORK_TCP_SOCKET_H

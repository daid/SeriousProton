#ifndef SP2_IO_NETWORK_TCP_SOCKET_H
#define SP2_IO_NETWORK_TCP_SOCKET_H

#include <io/network/address.h>
#include <io/network/socketBase.h>
#include <io/dataBuffer.h>


namespace sp {
namespace io {
namespace network {


class TcpSocket : public SocketBase
{
public:
    TcpSocket();
    TcpSocket(TcpSocket&& socket);
    ~TcpSocket();

    TcpSocket& operator=(TcpSocket&& other);

    bool connect(const Address& host, int port);
    bool connectSSL(const Address& host, int port);
    void close();

    bool isConnected();

    void send(const void* data, size_t size);
    size_t receive(void* data, size_t size);

    void send(const io::DataBuffer& buffer);
    bool receive(io::DataBuffer& buffer);

private:
    bool sendSendQueue();
    
    void* ssl_handle;

    std::string send_queue;
    std::vector<uint8_t> receive_buffer;
    size_t received_size;
    
    friend class TcpListener;
};

}//namespace network
}//namespace io
}//namespace sp

#endif//SP2_IO_NETWORK_TCP_SOCKET_H

#ifndef SP2_IO_NETWORK_STREAM_SOCKET_H
#define SP2_IO_NETWORK_STREAM_SOCKET_H

#include <io/dataBuffer.h>
#include <nonCopyable.h>


namespace sp {
namespace io {
namespace network {


class StreamSocket : sp::NonCopyable
{
public:
    enum class State {
        Closed,
        Connecting,
        Connected
    };
    virtual ~StreamSocket();
    virtual void close() = 0;

    virtual State getState() = 0;

    void send(const void* data, size_t size);
    void queue(const void* data, size_t size);
    size_t receive(void* data, size_t size);

    void send(const io::DataBuffer& buffer);
    void queue(const io::DataBuffer& buffer);
    bool receive(io::DataBuffer& buffer);

    //Returns true if there is still data in the queue after sending
    bool sendSendQueue();

protected:
    void clearQueue();

    virtual size_t _send(const void* data, size_t size) = 0;
    virtual size_t _receive(void* data, size_t size) = 0;
private:
    std::string send_queue;
    uint32_t receive_packet_size{0};
    bool receive_packet_size_done{false};
    std::vector<uint8_t> receive_buffer;
    size_t received_size{0};
};

}//namespace network
}//namespace io
}//namespace sp

#endif//SP2_IO_NETWORK_STREAM_SOCKET_H

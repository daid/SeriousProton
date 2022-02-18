#include <io/network/streamSocket.h>
#include <logging.h>


namespace sp {
namespace io {
namespace network {


StreamSocket::~StreamSocket()
{
}

void StreamSocket::send(const void* data, size_t size)
{
    if (getState() != State::Connected)
        return;
    if (sendSendQueue())
    {
        queue(data, size);
        return;
    }

    for(size_t done = 0; done < size; )
    {
        size_t result = _send(static_cast<const char*>(data) + done, static_cast<int>(size - done));
        if (result == 0)
        {
            if (getState() == State::Connected)
                send_queue += std::string(static_cast<const char*>(data) + done, size - done);
            return;
        }
        done += result;
    }
}

void StreamSocket::queue(const void* data, size_t size)
{
    send_queue += std::string(static_cast<const char*>(data), size);
}

size_t StreamSocket::receive(void* data, size_t size)
{
    sendSendQueue();
    
    if (getState() != State::Connected)
        return 0;
    
    return _receive(data, size);
}

void StreamSocket::send(const io::DataBuffer& buffer)
{
    io::DataBuffer packet_size(uint32_t(buffer.getDataSize()));
    send(packet_size.getData(), packet_size.getDataSize());
    send(buffer.getData(), buffer.getDataSize());
}

void StreamSocket::queue(const io::DataBuffer& buffer)
{
    io::DataBuffer packet_size(uint32_t(buffer.getDataSize()));
    queue(packet_size.getData(), packet_size.getDataSize());
    queue(buffer.getData(), buffer.getDataSize());
}

bool StreamSocket::receive(io::DataBuffer& buffer)
{
    if (getState() != State::Connected)
        return 0;
    
    if (!receive_packet_size_done)
    {
        uint8_t size_buffer[1];
        do {
            auto result = _receive(reinterpret_cast<char*>(size_buffer), 1);
            if (result == 0)
                return false;
            receive_packet_size = (receive_packet_size << 7) | (size_buffer[0] & 0x7F);
            if (!(size_buffer[0] & 0x80))
                receive_packet_size_done = true;
        } while(!receive_packet_size_done);

        receive_buffer.resize(receive_packet_size);
        receive_packet_size = 0;
        received_size = 0;
    }

    while(true)
    {
        auto result = _receive(reinterpret_cast<char*>(&receive_buffer[received_size]), static_cast<int>(receive_buffer.size() - received_size));
        received_size += result;
        if (received_size == receive_buffer.size())
        {
            buffer = std::move(receive_buffer);
            received_size = 0;
            receive_packet_size_done = false;
            return true;
        }
        if (result < 1)
            break;
    }
    
    return false;
}

bool StreamSocket::sendSendQueue()
{
    if (send_queue.size() < 1)
        return false;
    
    size_t result;
    do
    {
        result = _send(static_cast<const char*>(send_queue.data()), static_cast<int>(send_queue.size()));
        if (result > 0)
            send_queue = send_queue.substr(result);
    } while(result > 0 && send_queue.size() > 0);
    
    return send_queue.size() > 0;
}

void StreamSocket::clearQueue()
{
    send_queue.clear();
    receive_packet_size = 0;
    receive_packet_size_done = false;
    receive_buffer.clear();
    received_size = 0;
}

}//namespace network
}//namespace io
}//namespace sp

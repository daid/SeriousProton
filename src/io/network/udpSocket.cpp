#include <io/network/udpSocket.h>

#ifdef __WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
static constexpr int flags = 0;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>
static constexpr int flags = MSG_NOSIGNAL;
#endif


namespace sp {
namespace io {
namespace network {


UdpSocket::UdpSocket()
{
}

UdpSocket::UdpSocket(UdpSocket&& socket)
{
    handle = socket.handle;
    blocking = socket.blocking;

    socket.handle = -1;
}

UdpSocket::~UdpSocket()
{
    close();
}

bool UdpSocket::bind(int port)
{
    close();
    
    if (!createSocket())
        return false;

    if (socket_is_ipv6)
    {
        socket_is_ipv6 = true;

        struct sockaddr_in6 server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin6_family = AF_INET6;
        server_addr.sin6_addr = in6addr_any;
        server_addr.sin6_port = htons(port);

        if (::bind(handle, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0)
        {
            close();
            return false;
        }
    }
    else
    {
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        server_addr.sin_port = htons(port);

        if (::bind(handle, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0)
        {
            close();
            return false;
        }
    }
    return true;
}

bool UdpSocket::joinMulticast(int group_nr)
{
    if (handle == -1)
    {
        if (!createSocket())
            return false;
    }
    
    bool success = true;
    struct sockaddr_in server_addr;
    for(const auto& addr_info : Address::getLocalAddress().addr_info)
    {
        if (addr_info.family == AF_INET && addr_info.addr.length() == sizeof(server_addr))
        {
            memcpy(&server_addr, addr_info.addr.data(), addr_info.addr.length());
            
            struct ip_mreq mreq;
            mreq.imr_multiaddr.s_addr = htonl((239 << 24) | (192 << 16) | (group_nr));
            mreq.imr_interface.s_addr = server_addr.sin_addr.s_addr;
            
            success = success && ::setsockopt(handle, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<const char*>(&mreq), sizeof(mreq)) == 0;
        }
    }
    return success;
}

void UdpSocket::close()
{
    if (handle != -1)
    {
#ifdef __WIN32
        closesocket(handle);
#else
        ::close(handle);
#endif
        handle = -1;
    }
}

bool UdpSocket::send(const void* data, size_t size, const Address& address, int port)
{
    if (handle == -1)
    {
        if (!createSocket())
            return false;
    }
    
    if (socket_is_ipv6)
    {
        struct sockaddr_in6 server_addr;
        bool is_set = false;
        memset(&server_addr, 0, sizeof(server_addr));
        for(auto& addr_info : address.addr_info)
        {
            if (addr_info.family == AF_INET6 && addr_info.addr.length() == sizeof(server_addr))
            {
                memcpy(&server_addr, addr_info.addr.data(), addr_info.addr.length());
                is_set = true;
                break;
            }
        }
        
        if (is_set)
        {
            server_addr.sin6_family = AF_INET6;
            server_addr.sin6_port = htons(port);

            int result = ::sendto(handle, static_cast<const char*>(data), size, flags, reinterpret_cast<const sockaddr*>(&server_addr), sizeof(server_addr));
            return result == int(size);
        }
    }

    struct sockaddr_in server_addr;
    bool is_set = false;
    memset(&server_addr, 0, sizeof(server_addr));
    for(auto& addr_info : address.addr_info)
    {
        if (addr_info.family == AF_INET && addr_info.addr.length() == sizeof(server_addr))
        {
            memcpy(&server_addr, addr_info.addr.data(), addr_info.addr.length());
            is_set = true;
            break;
        }
    }

    if (is_set)
    {
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);

        int result = ::sendto(handle, static_cast<const char*>(data), size, flags, reinterpret_cast<const sockaddr*>(&server_addr), sizeof(server_addr));
        return result == int(size);
    }
    return false;
}

size_t UdpSocket::receive(void* data, size_t size, Address& address, int& port)
{
    if (handle == -1)
        return 0;

    address.addr_info.clear();
    if (socket_is_ipv6)
    {
        struct sockaddr_in6 from_addr;
        memset(&from_addr, 0, sizeof(from_addr));
        socklen_t from_addr_len = sizeof(from_addr);
        int result = ::recvfrom(handle, static_cast<char*>(data), size, flags, reinterpret_cast<sockaddr*>(&from_addr), &from_addr_len);
        if (result >= 0)
        {
            if (from_addr_len == sizeof(from_addr))
            {
                char buffer[128];
                ::getnameinfo(reinterpret_cast<const sockaddr*>(&from_addr), from_addr_len, buffer, sizeof(buffer), nullptr, 0, NI_NUMERICHOST);
                address.addr_info.emplace_back(AF_INET6, buffer, &from_addr, from_addr_len);
                port = ntohs(from_addr.sin6_port);
            }
            else if (from_addr_len == sizeof(struct sockaddr_in))
            {
                char buffer[128];
                ::getnameinfo(reinterpret_cast<const sockaddr*>(&from_addr), from_addr_len, buffer, sizeof(buffer), nullptr, 0, NI_NUMERICHOST);
                address.addr_info.emplace_back(AF_INET, buffer, &from_addr, from_addr_len);
                port = ntohs(reinterpret_cast<struct sockaddr_in*>(&from_addr)->sin_port);
            }
            return result;
        }
    }
    else
    {
        struct sockaddr_in from_addr;
        memset(&from_addr, 0, sizeof(from_addr));
        socklen_t from_addr_len = sizeof(from_addr);
        int result = ::recvfrom(handle, static_cast<char*>(data), size, flags, reinterpret_cast<sockaddr*>(&from_addr), &from_addr_len);
        if (result >= 0)
        {
            if (from_addr_len == sizeof(from_addr))
            {
                char buffer[128];
                ::getnameinfo(reinterpret_cast<const sockaddr*>(&from_addr), from_addr_len, buffer, sizeof(buffer), nullptr, 0, NI_NUMERICHOST);
                address.addr_info.emplace_back(AF_INET, buffer, &from_addr, from_addr_len);
                port = ntohs(from_addr.sin_port);
            }
            return result;
        }
    }
    return 0;
}

bool UdpSocket::send(const DataBuffer& buffer, const Address& address, int port)
{
    return send(buffer.getData(), buffer.getDataSize(), address, port);
}

bool UdpSocket::receive(DataBuffer& buffer, Address& address, int& port)
{
    uint8_t receive_buffer[4096];
    size_t received_size = receive(receive_buffer, sizeof(receive_buffer), address, port);

    if (received_size > 0)
    {
        buffer = std::vector<uint8_t>(receive_buffer, receive_buffer + received_size);
    }
    return received_size > 0;
}

bool UdpSocket::sendMulticast(const void* data, size_t size, int group_nr, int port)
{
    if (handle == -1)
    {
        createSocket();
    }

    bool success = false;
    struct sockaddr_in server_addr;
    for(const auto& addr_info : Address::getLocalAddress().addr_info)
    {
        if (addr_info.family == AF_INET && addr_info.addr.length() == sizeof(server_addr))
        {
            memcpy(&server_addr, addr_info.addr.data(), addr_info.addr.length());
            
            ::setsockopt(handle, IPPROTO_IP, IP_MULTICAST_IF, reinterpret_cast<const char*>(&server_addr.sin_addr), sizeof(server_addr.sin_addr));

            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin_addr.s_addr = htonl((239 << 24) | (192 << 16) | (group_nr));
            
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port);

            int result = ::sendto(handle, static_cast<const char*>(data), size, flags, reinterpret_cast<const sockaddr*>(&server_addr), sizeof(server_addr));
            if (result == int(size))
                success = true;
        }
    }
    return success;
}

bool UdpSocket::sendMulticast(const DataBuffer& buffer, int group_nr, int port)
{
    return sendMulticast(buffer.getData(), buffer.getDataSize(), group_nr, port);
}

bool UdpSocket::createSocket()
{
    close();

    handle = ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    socket_is_ipv6 = true;
    if (handle == -1)
    {
        handle = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        socket_is_ipv6 = false;
        
    }
    else
    {
        int optval = 0;
        ::setsockopt(handle, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<const char*>(&optval), sizeof(int));
    }
    return handle != -1;
}

}//namespace network
}//namespace io
}//namespace sp

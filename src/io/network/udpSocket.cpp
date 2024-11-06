#include <io/network/udpSocket.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
static constexpr int flags = 0;

static inline int recvfrom(SOCKET s, void* buf, size_t len, int flags, struct sockaddr* from, socklen_t* fromlen)
{
    return recvfrom(s, static_cast<char*>(buf), static_cast<int>(len), flags, from, fromlen);
}

static inline int sendto(SOCKET s, const void* msg, size_t len, int flags, const struct sockaddr* to, socklen_t tolen)
{
    return sendto(s, static_cast<const char*>(msg), static_cast<int>(len), flags, to, tolen);
}
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>
#if defined(__APPLE__)
static constexpr int flags = 0;
#else
static constexpr int flags = MSG_NOSIGNAL;
#endif
static constexpr intptr_t INVALID_SOCKET = -1;
// Define IPV6_ADD_MEMBERSHIP for FreeBSD and Mac OS X
#ifndef IPV6_ADD_MEMBERSHIP
#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#endif
#endif


namespace sp {
namespace io {
namespace network {


UdpSocket::UdpSocket()
{
}

UdpSocket::UdpSocket(UdpSocket&& socket)
{
    if (this == &socket)
        return;

    handle = socket.handle;
    blocking = socket.blocking;

    socket.handle = INVALID_SOCKET;
}

UdpSocket::~UdpSocket()
{
    close();
}

bool UdpSocket::bind(int port)
{
    initSocketLib();
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
    if (handle == INVALID_SOCKET)
    {
        if (!createSocket())
            return false;
    }
    
    bool success = true;
    if (!socket_is_ipv6)
    {
        for(const auto& addr_info : Address::getLocalAddress().addr_info)
        {
            if (addr_info.family == AF_INET && !socket_is_ipv6 && addr_info.addr.size() == sizeof(struct sockaddr_in))
            {
                struct sockaddr_in server_addr;
                memcpy(&server_addr, addr_info.addr.data(), addr_info.addr.size());
                
                struct ip_mreq mreq;
                mreq.imr_multiaddr.s_addr = htonl((239 << 24) | (192 << 16) | (group_nr));
                mreq.imr_interface.s_addr = server_addr.sin_addr.s_addr;
                
                success = success && ::setsockopt(handle, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<const char*>(&mreq), sizeof(mreq)) == 0;
            }
        }
    }
    else
    {
        struct ipv6_mreq mreq;
        mreq.ipv6mr_interface = 0;
        mreq.ipv6mr_multiaddr.s6_addr[0] = 0xff;
        mreq.ipv6mr_multiaddr.s6_addr[1] = 0x08;
        mreq.ipv6mr_multiaddr.s6_addr[14] = group_nr >> 8;
        mreq.ipv6mr_multiaddr.s6_addr[15] = group_nr;

        success = ::setsockopt(handle, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, reinterpret_cast<const char*>(&mreq), sizeof(mreq)) == 0;
    }
    return success;
}

void UdpSocket::close()
{
    if (handle != INVALID_SOCKET)
    {
#ifdef _WIN32
        closesocket(handle);
#else
        ::close(handle);
#endif
        handle = INVALID_SOCKET;
    }
}

bool UdpSocket::send(const void* data, size_t size, const Address& address, int port)
{
    if (handle == INVALID_SOCKET)
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
            if (addr_info.family == AF_INET6 && addr_info.addr.size() == sizeof(server_addr))
            {
                memcpy(&server_addr, addr_info.addr.data(), addr_info.addr.size());
                is_set = true;
                break;
            }
        }
        
        if (is_set)
        {
            server_addr.sin6_family = AF_INET6;
            server_addr.sin6_port = htons(port);

            int result = ::sendto(handle, data, size, flags, reinterpret_cast<const sockaddr*>(&server_addr), sizeof(server_addr));
            return result == int(size);
        }
    }

    struct sockaddr_in server_addr;
    bool is_set = false;
    memset(&server_addr, 0, sizeof(server_addr));
    for(auto& addr_info : address.addr_info)
    {
        if (addr_info.family == AF_INET && addr_info.addr.size() == sizeof(server_addr))
        {
            memcpy(&server_addr, addr_info.addr.data(), addr_info.addr.size());
            is_set = true;
            break;
        }
    }

    if (is_set)
    {
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);

        int result = ::sendto(handle, data, size, flags, reinterpret_cast<const sockaddr*>(&server_addr), sizeof(server_addr));
        return result == int(size);
    }
    return false;
}

size_t UdpSocket::receive(void* data, size_t size, Address& address, int& port)
{
    if (handle == INVALID_SOCKET)
        return 0;

    address.addr_info.clear();
    if (socket_is_ipv6)
    {
        struct sockaddr_in6 from_addr;
        memset(&from_addr, 0, sizeof(from_addr));
        socklen_t from_addr_len = sizeof(from_addr);
        int result = ::recvfrom(handle, data, size, flags, reinterpret_cast<sockaddr*>(&from_addr), &from_addr_len);
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
        int result = ::recvfrom(handle, data, size, flags, reinterpret_cast<sockaddr*>(&from_addr), &from_addr_len);
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
    if (handle == INVALID_SOCKET)
    {
        createSocket();
    }

    bool success = false;
    for(const auto& addr_info : Address::getLocalAddress().addr_info)
    {
        if (addr_info.family == AF_INET && !socket_is_ipv6 && addr_info.addr.size() == sizeof(struct sockaddr_in))
        {
            struct sockaddr_in server_addr;
            memcpy(&server_addr, addr_info.addr.data(), addr_info.addr.size());
            
            ::setsockopt(handle, IPPROTO_IP, IP_MULTICAST_IF, reinterpret_cast<const char*>(&server_addr.sin_addr), sizeof(server_addr.sin_addr));

            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin_addr.s_addr = htonl((239 << 24) | (192 << 16) | (group_nr));
            
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port);

            int result = ::sendto(handle, data, size, flags, reinterpret_cast<const sockaddr*>(&server_addr), sizeof(server_addr));
            if (result == int(size))
                success = true;
        }
        if (addr_info.family == AF_INET6 && socket_is_ipv6 && addr_info.addr.size() == sizeof(struct sockaddr_in6))
        {
            struct sockaddr_in6 server_addr;
            memcpy(&server_addr, addr_info.addr.data(), addr_info.addr.size());
            
            ::setsockopt(handle, IPPROTO_IPV6, IPV6_MULTICAST_IF, reinterpret_cast<const char*>(&server_addr.sin6_addr), sizeof(server_addr.sin6_addr));

            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin6_addr.s6_addr[0] = 0xff;
            server_addr.sin6_addr.s6_addr[1] = 0x08;
            server_addr.sin6_addr.s6_addr[14] = group_nr >> 8;
            server_addr.sin6_addr.s6_addr[15] = group_nr;
            server_addr.sin6_family = AF_INET6;
            server_addr.sin6_port = htons(port);

            int result = ::sendto(handle, data, size, flags, reinterpret_cast<const sockaddr*>(&server_addr), sizeof(server_addr));
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

bool UdpSocket::sendBroadcast(const void* data, size_t size, int port)
{
    if (handle == INVALID_SOCKET)
        return false;
    if (socket_is_ipv6)
        return false;

#ifdef _WIN32
    bool success = false;
    //On windows, using a single broadcast address seems to send the UDP packets only on 1 interface.
    // So use the windows API to get all addresses, construct broadcast addresses and send out the packets to all of them.
    PMIB_IPADDRTABLE pIPAddrTable;
    DWORD tableSize = 0;
    GetIpAddrTable(NULL, &tableSize, 0);
    if (tableSize > 0)
    {
        pIPAddrTable = (PMIB_IPADDRTABLE)calloc(tableSize, 1);
        if (GetIpAddrTable(pIPAddrTable, &tableSize, 0) == NO_ERROR)
        {
            for(unsigned int n=0; n<pIPAddrTable->dwNumEntries; n++)
            {
                struct sockaddr_in s;
                memset(&s, '\0', sizeof(struct sockaddr_in));
                s.sin_family = AF_INET;
                s.sin_port = htons(port);
                s.sin_addr.s_addr = (pIPAddrTable->table[n].dwAddr & pIPAddrTable->table[n].dwMask) | ~pIPAddrTable->table[n].dwMask;
                if (::sendto(handle, data, size, 0, (struct sockaddr*)&s, sizeof(struct sockaddr_in)) >= 0)
                    success = true;
            }
        }
        free(pIPAddrTable);
    }
    return success;
#else
    struct sockaddr_in s;
    memset(&s, '\0', sizeof(struct sockaddr_in));
    s.sin_family = AF_INET;
    s.sin_port = htons(port);
    s.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    return ::sendto(handle, (const char*)data, size, 0, (struct sockaddr*)&s, sizeof(struct sockaddr_in)) >= 0;
#endif
}

bool UdpSocket::sendBroadcast(const DataBuffer& buffer, int port)
{
    return sendBroadcast(buffer.getData(), buffer.getDataSize(), port);
}

bool UdpSocket::createSocket()
{
    close();

    //As IPv6 multicast isn't working yet, just use IPv4 sockets only right now.
    //handle = ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    handle = INVALID_SOCKET;
    socket_is_ipv6 = true;
    if (handle == INVALID_SOCKET)
    {
        handle = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        socket_is_ipv6 = false;
    }
    else
    {
        //Make sure the ipv6 socket also does ipv4
        int optval = 0;
        ::setsockopt(handle, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<const char*>(&optval), sizeof(int));
    }
    //Enable broadcasting
    int enable = 1;
    ::setsockopt(handle, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char*>(&enable), sizeof(enable));

    return handle != INVALID_SOCKET;
}

}//namespace network
}//namespace io
}//namespace sp

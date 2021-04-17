#include <string.h>
#include <stdint.h>
#ifdef _WIN32
#include <winsock2.h>
#include <iphlpapi.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

#ifdef __linux__
#define SOCKET_FLAGS MSG_NOSIGNAL
#else
#define SOCKET_FLAGS 0
#endif

#include "fixedSocket.h"
#include "logging.h"

TcpSocket::TcpSocket()
{
    backlog_data_block = NULL;
    backlog_data_block_size = 0;
}

TcpSocket::~TcpSocket()
{
    if (backlog_data_block)
        delete[] backlog_data_block;
}

sf::Socket::Status TcpSocket::send(sf::Packet& packet)
{
    if (backlog_data_block || !send_backlog.empty())
    {
        if (send_backlog.empty())
            backlog_clock.restart();
        send_backlog.push_back(packet);
        return sf::Socket::Done;
    }
    private_send(packet);
    return sf::Socket::Done;
}

void TcpSocket::private_send(sf::Packet& packet)
{    
    auto size = static_cast<int>(packet.getDataSize());
    const void* data = packet.getData();
    
    sf::Uint32 packetSize = htonl(static_cast<sf::Uint32>(size));
    int sent = ::send(getHandle(), reinterpret_cast<const char*>(&packetSize), sizeof(packetSize), SOCKET_FLAGS);
    if (sent < 0)   //Note: No disconnect caught here. It will be caught in the receive call.
        sent = 0;
    if (sent < static_cast<int>(sizeof(packetSize)))
    {
        backlog_data_block_size = sizeof(packetSize) - sent + size;
        backlog_data_block = new uint8_t[backlog_data_block_size];
        memcpy(backlog_data_block, reinterpret_cast<const char*>(&packetSize) + sent, sizeof(packetSize) - sent);
        memcpy(backlog_data_block + sizeof(packetSize) - sent, data, size);
        return;
    }
    sent = ::send(getHandle(), reinterpret_cast<const char*>(data), size, SOCKET_FLAGS);
    if (sent < 0)   //Note: No disconnect caught here. It will be caught in the receive call.
        sent = 0;
    if (sent < size)
    {
        backlog_data_block_size = size - sent;
        backlog_data_block = new uint8_t[backlog_data_block_size];
        memcpy(backlog_data_block, reinterpret_cast<const char*>(data) + sent, backlog_data_block_size);
        return;
    }
}

void TcpSocket::update()
{
    if (backlog_data_block)
    {
        int sent = ::send(getHandle(), reinterpret_cast<const char*>(backlog_data_block), backlog_data_block_size, SOCKET_FLAGS);
        if (sent < 1)   //Note: No disconnect caught here. It will be caught in the receive call.
            return;
        if (sent < backlog_data_block_size)
        {
            uint8_t* new_block = new uint8_t[backlog_data_block_size - sent];
            memcpy(new_block, backlog_data_block + sent, backlog_data_block_size - sent);
            delete[] backlog_data_block;
            backlog_data_block = new_block;
            return;
        }
        delete[] backlog_data_block;
        backlog_data_block = NULL;
    }
    
    if (!send_backlog.empty())
    {
        while(!backlog_data_block && !send_backlog.empty())
        {
            private_send(send_backlog.front());
            send_backlog.pop_front();
        }
        if (backlog_clock.getElapsedTime().asSeconds() > 20.0)
        {
            LOG(DEBUG) << "Socket backlog clock timeout, disconnecting.";
            disconnect();
        }
    }
}

void UDPbroadcastPacket(sf::UdpSocket& socket, sf::Packet packet, int port_nr)
{
#ifdef _WIN32
    //On windows, using a single broadcast address seems to send the UPD package only on 1 interface.
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
                sf::IpAddress ip(ntohl((pIPAddrTable->table[n].dwAddr & pIPAddrTable->table[n].dwMask) | ~pIPAddrTable->table[n].dwMask));
                socket.send(packet, ip, static_cast<uint16_t>(port_nr));
            }
        }
        free(pIPAddrTable);
    }
#else
    socket.send(packet, sf::IpAddress::Broadcast, port_nr);
#endif
}

void UDPbroadcastPacket(sf::UdpSocket& socket, const void* data, std::size_t size, int port_nr)
{
#ifdef _WIN32
    //On windows, using a single broadcast address seems to send the UPD package only on 1 interface.
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
                sf::IpAddress ip(ntohl((pIPAddrTable->table[n].dwAddr & pIPAddrTable->table[n].dwMask) | ~pIPAddrTable->table[n].dwMask));
                socket.send(data, size, ip, static_cast<uint16_t>(port_nr));
            }
        }
        free(pIPAddrTable);
    }
#else
    socket.send(data, size, sf::IpAddress::Broadcast, port_nr);
#endif
}

#ifndef FIXED_SOCKET_H
#define FIXED_SOCKET_H

#include <SFML/Network.hpp>
#include <list>

//The sf::TcpSocket has some problems with sending partial packets when the the OS buffers are full.
class TcpSocket : public sf::TcpSocket
{
private:
    std::list<sf::Packet> send_backlog;
    sf::Clock backlog_clock;
    uint8_t* backlog_data_block;
    int backlog_data_block_size;
public:
    TcpSocket();
    ~TcpSocket();
    
    sf::TcpSocket::Status send(sf::Packet &packet);
    
    void update();
private:
    void private_send(sf::Packet &packet);
};

//Utility function to assist in broadcasting UDP packets.
void UDPbroadcastPacket(sf::UdpSocket& socket, sf::Packet packet, int port_nr);

#endif//FIXED_SOCKET_H

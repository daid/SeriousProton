#ifndef FIXED_SOCKET_H
#define FIXED_SOCKET_H

#include <SFML/Network.hpp>
#include <list>

//The sf::TcpSocket has some problems with sending partial packets when the the OS buffers are full.
//The TcpSocket of SeriousProton solves this by storing the partial packet in a buffer, and storing all other packets after that as well.
//This requires the call to TcpSocket::update() in an attempt to resend this data properly.
//NOTE: This has been fixed in SFML-2.3 with the addition of sf::Socket::Status::Partial, but we're still using our old code, which is known to work.
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
void UDPbroadcastPacket(sf::UdpSocket& socket, const void* data, std::size_t size, int port_nr);

#endif//FIXED_SOCKET_H

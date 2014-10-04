#ifndef MULTIPLAYER_SERVER_SCANER_H
#define MULTIPLAYER_SERVER_SCANER_H

#include "multiplayer_server.h"

//Class to find all servers that have the correct version number. Creates a big nice list.
class ServerScanner : public Updatable
{
    sf::Clock updateTimeClock;
    int serverPort;
    sf::UdpSocket socket;
    float broadcastDelay;

public:
    struct ServerInfo
    {
        sf::IpAddress address;
        float timeout;
        string name;
    };
private:

    std::vector<struct ServerInfo> serverList;
    int versionNumber;
    const static float serverTimeout = 30.0;
public:

    ServerScanner(int versionNumber, int serverPort = defaultServerPort);

    virtual void update(float delta);

    std::vector<ServerInfo> getServerList();
    
    //Utility function to assist in broadcasting UDP packets.
    static void broadcastPacket(sf::UdpSocket& socket, sf::Packet packet, int port_nr);
};

#endif//MULTIPLAYER_SERVER_SCANER_H

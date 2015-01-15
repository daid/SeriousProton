#include "multiplayer_server_scanner.h"

ServerScanner::ServerScanner(int versionNumber, int serverPort)
: serverPort(serverPort), versionNumber(versionNumber)
{
    int portNr = serverPort + 1;
    while(socket.bind(portNr) != sf::UdpSocket::Done)
        portNr++;
    
    socket.setBlocking(false);
    broadcastDelay = 0.0;
}

void ServerScanner::update(float gameDelta)
{
    //Calculate our own delta, as we want wall-time delta, the gameDelta can be modified by the current game speed (could even be 0 on pause)
    float delta = updateTimeClock.getElapsedTime().asSeconds();
    updateTimeClock.restart();

    broadcastDelay -= delta;
    if (broadcastDelay < 0.0)
    {
        sf::Packet sendPacket;
        sendPacket << multiplayerVerficationNumber << "ServerQuery" << int32_t(versionNumber);
        UDPbroadcastPacket(socket, sendPacket, serverPort);
        broadcastDelay += 2.0;
    }
    
    for(unsigned int n=0; n<serverList.size(); n++)
    {
        serverList[n].timeout += delta;
        if (serverList[n].timeout > serverTimeout)
        {
            serverList.erase(serverList.begin() + n);
            n--;
        }
    }

    sf::IpAddress recvAddress;
    unsigned short recvPort;
    sf::Packet recvPacket;
    if (socket.receive(recvPacket, recvAddress, recvPort) == sf::UdpSocket::Done)
    {
        int32_t verification, versionNr;
        string name;
        recvPacket >> verification >> versionNr >> name;
        if (verification == multiplayerVerficationNumber && (versionNr == versionNumber || versionNr == 0 || versionNumber == 0))
        {
            bool found = false;
            for(unsigned int n=0; n<serverList.size(); n++)
            {
                if (serverList[n].address == recvAddress)
                {
                    serverList[n].timeout = 0.0;
                    serverList[n].name = name;
                    found = true;
                }
            }
            if (!found)
            {
                printf("ServerScanner::New server: %s %i %s\n", recvAddress.toString().c_str(), recvPort, name.c_str());
                ServerInfo si;
                si.address = recvAddress;
                si.timeout = 0.0;
                si.name = name;
                serverList.push_back(si);
            }
        }
    }
}

std::vector<ServerScanner::ServerInfo> ServerScanner::getServerList()
{
    return serverList;
}

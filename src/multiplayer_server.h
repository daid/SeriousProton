#ifndef MULTIPLAYER_SERVER_H
#define MULTIPLAYER_SERVER_H

#include <SFML/Network.hpp>
#include "Updatable.h"
#include "stringImproved.h"

static const int defaultServerPort = 35666;
static const int multiplayerVerficationNumber = 0x2fab3f0f; //Used to verify that the server is actually a serious proton server

class GameServer;
class MultiplayerObject;

extern P<GameServer> game_server;

class GameServer : public Updatable
{
    sf::Clock updateTimeClock;
    sf::UdpSocket broadcastListenSocket;
    sf::TcpListener listenSocket;
    sf::SocketSelector selector;
    string serverName;
    int versionNumber;
    int sendDataCounter;
    float sendDataRate;
    float lastGameSpeed;

    enum EClientReceiveState
    {
        CRS_Main,
        CRS_Command
    };
    struct ClientInfo
    {
        sf::TcpSocket* socket;
        int32_t clientId;
        EClientReceiveState receiveState;
        int32_t command_object_id;
        std::vector<sf::Packet> packet_backlog;
        sf::Clock backlog_clock;
    };
    int32_t nextClientId;
    std::vector<ClientInfo> clientList;

    int32_t nextObjectId;
    std::map<int32_t, P<MultiplayerObject> > objectMap;
public:
    GameServer(string serverName, int versionNumber, int listenPort = defaultServerPort);

    P<MultiplayerObject> getObjectById(int32_t id);
    virtual void update(float delta);
    inline float getSendDataRate() { return sendDataRate; }

    string getServerName() { return serverName; }
    void setServerName(string name) { serverName = name; }

private:
    void registerObject(P<MultiplayerObject> obj);
    void sendAll(sf::Packet& packet);

    void genenerateCreatePacketFor(P<MultiplayerObject> obj, sf::Packet& packet);
    void genenerateDeletePacketFor(int32_t id, sf::Packet& packet);

    friend class MultiplayerObject;
public:
    virtual void onNewClient(int32_t clientId) {}
    virtual void onDisconnectClient(int32_t clientId) {}
};

#endif//MULTIPLAYER_SERVER_H

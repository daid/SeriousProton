#ifndef MULTIPLAYER_PROXY_H
#define MULTIPLAYER_PROXY_H

#include <memory>
#include "multiplayer_server.h"

class GameServerProxy : public Updatable
{
    sf::Clock lastReceiveTime;
    constexpr static float noDataDisconnectTime = 20.0f;

    sf::UdpSocket broadcast_listen_socket;
    sf::TcpListener listenSocket;
    std::unique_ptr<TcpSocket> newSocket;

    enum EClientReceiveState
    {
        CRS_Auth,
        CRS_Main,
        CRS_Command
    };
    struct ClientInfo
    {
        std::unique_ptr<TcpSocket> socket;
        int32_t clientId = 0;
        int32_t commandObjectId = 0;
        bool validClient = false;
        EClientReceiveState receiveState = CRS_Auth;
    };
    std::vector<ClientInfo> clientList;
    std::unordered_set<int32_t> targetClients;

    int32_t clientId = 0;
    string password;
    int32_t serverVersion = 0;
    string proxyName;
    float boardcastServerDelay;
    std::unique_ptr<TcpSocket> mainSocket;
public:
    GameServerProxy(sf::IpAddress hostname, int hostPort = defaultServerPort, string password = "", int listenPort = defaultServerPort, string proxyName="");
    GameServerProxy(string password = "", int listenPort = defaultServerPort, string proxyName="");
    virtual ~GameServerProxy();

    virtual void destroy() override;

    virtual void update(float delta) override;
private:
    void sendAll(sf::Packet& packet);

    void handleBroadcastUDPSocket(float delta);
};

#endif//MULTIPLAYER_PROXY_H

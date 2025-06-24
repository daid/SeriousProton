#ifndef MULTIPLAYER_PROXY_H
#define MULTIPLAYER_PROXY_H

#include <memory>
#include "multiplayer_server.h"

class GameServerProxy : public Updatable
{
    sp::SystemTimer heartbeat_timer;
    sp::SystemTimer no_data_timeout;

    // if the server doesn't send us any data for this long, send a packet to see if it's still there
    constexpr static float heartbeatTime = 0.5;
    // if the server doesn't send us any data for this long, disconnect
    constexpr static float noDataDisconnectTime = 20.0f;

    sp::io::network::UdpSocket broadcast_listen_socket;
    sp::io::network::TcpListener listenSocket;
    std::unique_ptr<sp::io::network::TcpSocket> newSocket;

    enum EClientReceiveState
    {
        CRS_Auth,
        CRS_Main,
        CRS_Command
    };
    struct ClientInfo
    {
        std::unique_ptr<sp::io::network::TcpSocket> socket;
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
    std::unique_ptr<sp::io::network::TcpSocket> mainSocket;
public:
    GameServerProxy(sp::io::network::Address hostname, int hostPort = defaultServerPort, string password = "", int listenPort = defaultServerPort, string proxyName="");
    GameServerProxy(string password = "", int listenPort = defaultServerPort, string proxyName="");
    virtual ~GameServerProxy();

    virtual void destroy() override;

    virtual void update(float delta) override;
private:
    void sendAll(sp::io::DataBuffer& packet);

    void handleBroadcastUDPSocket(float delta);
};

#endif//MULTIPLAYER_PROXY_H

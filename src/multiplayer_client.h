#ifndef MULTIPLAYER_CLIENT_H
#define MULTIPLAYER_CLIENT_H

#include <stdint.h>
#include "fixedSocket.h"
#include "Updatable.h"
#include "multiplayer_server.h"

class GameClient;
class MultiplayerObject;

extern P<GameClient> game_client;

class GameClient : public Updatable
{
    const static float no_data_disconnect_time = 20.0f;

    TcpSocket socket;
    std::map<int32_t, P<MultiplayerObject> > objectMap;
    int32_t client_id;
    bool connected;
    sf::Clock last_receive_time;
public:
    GameClient(sf::IpAddress server, int portNr = defaultServerPort);

    P<MultiplayerObject> getObjectById(int32_t id);
    virtual void update(float delta);

    int32_t getClientId() { return client_id; }
    bool isConnected() { return connected; }

    void sendPacket(sf::Packet& packet);
};

#endif//MULTIPLAYER_CLIENT_H

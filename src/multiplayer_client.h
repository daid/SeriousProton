#ifndef MULTIPLAYER_CLIENT_H
#define MULTIPLAYER_CLIENT_H

#include <SFML/Network.hpp>
#include "Updatable.h"
#include "multiplayer_server.h"

class GameClient;
class MultiplayerObject;

extern P<GameClient> gameClient;

class GameClient : public Updatable
{
    sf::TcpSocket socket;
    std::map<int32_t, P<MultiplayerObject> > objectMap;
    int32_t clientId;
    bool connected;
public:
    GameClient(sf::IpAddress server, int portNr = defaultServerPort);

    P<MultiplayerObject> getObjectById(int32_t id);
    virtual void update(float delta);

    int32_t getClientId() { return clientId; }
    bool isConnected() { return connected; }

    friend class MultiplayerObject;
};

#endif//MULTIPLAYER_CLIENT_H

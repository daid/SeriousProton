#include "multiplayer_client.h"
#include "multiplayer.h"
#include "multiplayer_internal.h"
#include "engine.h"

P<GameClient> game_client;

GameClient::GameClient(sf::IpAddress server, int portNr)
{
    assert(!game_server);
    assert(!game_client);

    client_id = -1;
    game_client = this;

    if (socket.connect(server, portNr, sf::seconds(5)) != sf::TcpSocket::Done)
        connected = false;
    else
        connected = true;
    last_receive_time.restart();
    socket.setBlocking(false);
}

P<MultiplayerObject> GameClient::getObjectById(int32_t id)
{
    if (objectMap.find(id) != objectMap.end())
        return objectMap[id];
    return NULL;
}

void GameClient::update(float delta)
{
    std::vector<int32_t> delList;
    for(std::map<int32_t, P<MultiplayerObject> >::iterator i=objectMap.begin(); i != objectMap.end(); i++)
    {
        int id = i->first;
        P<MultiplayerObject> obj = i->second;
        if (!obj)
            delList.push_back(id);
    }
    for(unsigned int n=0; n<delList.size(); n++)
        objectMap.erase(delList[n]);

    socket.update();
    sf::Packet packet;
    sf::TcpSocket::Status status;
    while((status = socket.receive(packet)) == sf::TcpSocket::Done)
    {
        last_receive_time.restart();

        command_t command;
        packet >> command;
        switch(command)
        {
        case CMD_CREATE:
            {
                int32_t id;
                string name;
                packet >> id >> name;
                for(MultiplayerClassListItem* i = multiplayerClassListStart; i; i = i->next)
                {
                    if (i->name == name)
                    {
                        LOG(INFO) << "Created " << name << " from server replication";
                        MultiplayerObject* obj = i->func();
                        obj->multiplayerObjectId = id;
                        objectMap[id] = obj;

                        int16_t idx;
                        while(packet >> idx)
                        {
                            if (idx < int16_t(obj->memberReplicationInfo.size()))
                                (obj->memberReplicationInfo[idx].receiveFunction)(obj->memberReplicationInfo[idx].ptr, packet);
                        }
                    }
                }
            }
            break;
        case CMD_DELETE:
            {
                int32_t id;
                packet >> id;
                if (objectMap.find(id) != objectMap.end() && objectMap[id])
                    objectMap[id]->destroy();
            }
            break;
        case CMD_UPDATE_VALUE:
            {
                int32_t id;
                int16_t idx;
                packet >> id;
                if (objectMap.find(id) != objectMap.end() && objectMap[id])
                {
                    P<MultiplayerObject> obj = objectMap[id];
                    while(packet >> idx)
                    {
                        if (idx < int32_t(obj->memberReplicationInfo.size()))
                            (obj->memberReplicationInfo[idx].receiveFunction)(obj->memberReplicationInfo[idx].ptr, packet);
                    }
                }
            }
            break;
        case CMD_SET_CLIENT_ID:
            packet >> client_id;
            break;
        case CMD_SET_GAME_SPEED:
            {
                float gamespeed;
                packet >> gamespeed;
                engine->setGameSpeed(gamespeed);
            }
            break;
        case CMD_ALIVE:
            //Alive packet, just to keep the connection alive.
            break;
        default:
            LOG(ERROR) << "Unknown command from server: " << command;
        }
    }

    if (status == sf::TcpSocket::Disconnected || last_receive_time.getElapsedTime().asSeconds() > no_data_disconnect_time)
    {
        socket.disconnect();
        connected = false;
    }
}

void GameClient::sendPacket(sf::Packet& packet)
{
    socket.send(packet);
}

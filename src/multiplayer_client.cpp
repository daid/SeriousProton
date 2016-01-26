#include "multiplayer_client.h"
#include "multiplayer.h"
#include "multiplayer_internal.h"
#include "engine.h"

P<GameClient> game_client;

GameClient::GameClient(int version_number, sf::IpAddress server, int port_nr)
: version_number(version_number), server(server), port_nr(port_nr), connect_thread(&GameClient::runConnect, this)
{
    assert(!game_server);
    assert(!game_client);

    client_id = -1;
    game_client = this;
    status = ReadyToConnect;

    last_receive_time.restart();
}

GameClient::~GameClient()
{
    connect_thread.wait();
}

P<MultiplayerObject> GameClient::getObjectById(int32_t id)
{
    if (objectMap.find(id) != objectMap.end())
        return objectMap[id];
    return NULL;
}

void GameClient::update(float delta)
{
    if (status == ReadyToConnect)
    {
        status = Connecting;
        connect_thread.launch();
    }
    if (status == Disconnected || status == Connecting)
        return;

    std::vector<int32_t> delList;
    for(std::unordered_map<int32_t, P<MultiplayerObject> >::iterator i=objectMap.begin(); i != objectMap.end(); i++)
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
    sf::TcpSocket::Status socket_status;
    while((socket_status = socket.receive(packet)) == sf::TcpSocket::Done)
    {
        last_receive_time.restart();

        command_t command;
        packet >> command;
        switch(status)
        {
        case ReadyToConnect:
        case Connecting:
        case Authenticating:
        case WaitingForPassword:
            switch(command)
            {
            case CMD_REQUEST_AUTH:
                {
                    int32_t server_version;
                    bool require_password;
                    packet >> server_version >> require_password;
                    
                    if (!require_password)
                    {
                        sf::Packet reply;
                        reply << CMD_CLIENT_SEND_AUTH << int32_t(version_number) << string("");
                        socket.send(reply);
                    }else{
                        status = WaitingForPassword;
                    }
                }
                break;
            case CMD_SET_CLIENT_ID:
                packet >> client_id;
                status = Connected;
                break;
            case CMD_ALIVE:
                //Alive packet, just to keep the connection alive.
                break;
            default:
                LOG(ERROR) << "Unknown command from server: " << command;
            }
            break;
        case Connected:
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
                                if (idx >= 0 && idx < int16_t(obj->memberReplicationInfo.size()))
                                    (obj->memberReplicationInfo[idx].receiveFunction)(obj->memberReplicationInfo[idx].ptr, packet);
                                else
                                    LOG(DEBUG) << "Odd index from server replication: " << idx;
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
        case Disconnected:
            break;
        }
    }

    if (socket_status == sf::TcpSocket::Disconnected || last_receive_time.getElapsedTime().asSeconds() > no_data_disconnect_time)
    {
        socket.disconnect();
        status = Disconnected;
    }
}

void GameClient::sendPacket(sf::Packet& packet)
{
    socket.send(packet);
}

void GameClient::sendPassword(string password)
{
    if (status != WaitingForPassword)
        return;

    sf::Packet reply;
    reply << CMD_CLIENT_SEND_AUTH << int32_t(version_number) << password;
    socket.send(reply);
    
    status = Authenticating;
}

void GameClient::runConnect()
{
    if (socket.connect(server, port_nr, sf::seconds(5)) == sf::TcpSocket::Done)
    {
        LOG(INFO) << "GameClient: Connected, waiting for authentication";
        status = Authenticating;
    }else{
        LOG(INFO) << "GameClient: Failed to connect";
        status = Disconnected;
    }
    socket.setBlocking(false);
}

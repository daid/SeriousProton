#include "multiplayer_client.h"
#include "multiplayer.h"
#include "multiplayer_internal.h"
#include "engine.h"

#include "io/network/tcpSocket.h"
#ifdef STEAMSDK
#include "io/network/steamP2PSocket.h"
#endif

P<GameClient> game_client;

GameClient::GameClient(int version_number, sp::io::network::Address server, int port_nr)
: version_number(version_number), server(server), port_nr(port_nr)
{
    SDL_assert(!game_server);
    SDL_assert(!game_client);

    client_id = -1;
    game_client = this;
    status = Connecting;

    no_data_timeout.start(no_data_disconnect_time);
    auto sock = std::make_unique<sp::io::network::TcpSocket>();
    sock->setBlocking(false);
    sock->connect(server, port_nr);
    socket = std::move(sock);
}

#ifdef STEAMSDK
GameClient::GameClient(int version_number, uint64_t steam_id)
{
    SDL_assert(!game_server);
    SDL_assert(!game_client);

    client_id = -1;
    game_client = this;
    status = Connecting;

    no_data_timeout.start(no_data_disconnect_time);
    auto sock = std::make_unique<sp::io::network::SteamP2PSocket>();
    sock->connect(steam_id);
    socket = std::move(sock);
}
#endif

GameClient::~GameClient()
{
}

P<MultiplayerObject> GameClient::getObjectById(int32_t id)
{
    if (objectMap.find(id) != objectMap.end())
        return objectMap[id];
    return NULL;
}

void GameClient::update(float /*delta*/)
{
    if (status == Disconnected)
        return;
    if (status == Connecting)
    {
        switch(socket->getState())
        {
        case sp::io::network::StreamSocket::State::Closed:
            status = Disconnected;
            disconnect_reason = DisconnectReason::FailedToConnect;
            LOG(INFO) << "GameClient: Failed to connect";
            break;
        case sp::io::network::StreamSocket::State::Connecting:
            break;
        case sp::io::network::StreamSocket::State::Connected:
            LOG(INFO) << "GameClient: Connected, waiting for authentication";
            status = Authenticating;
            break;
        }
        return;
    }

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

    sp::io::DataBuffer reply;
    sp::io::DataBuffer packet;
    while(socket->receive(packet))
    {
        no_data_timeout.start(no_data_disconnect_time);

        command_t command;
        packet >> command;
        switch(status)
        {
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

                    if (server_version != 0 && server_version != version_number)
                    {
                        require_password = false; // forcibly send an empty pass, we're about to fail anyway.
                        disconnect_reason = DisconnectReason::VersionMismatch;
                        LOG(INFO) << "Server version " << server_version << " does not match client version " << version_number;
                    }

                    if (!require_password)
                    {
                        reply.clear();
                        reply << CMD_CLIENT_SEND_AUTH << int32_t(version_number) << string("");
                        socket->send(reply);
                    }else{
                        status = WaitingForPassword;
                    }
                }
                break;
            case CMD_SET_CLIENT_ID:
                packet >> client_id;
                status = Connected;
                disconnect_reason = DisconnectReason::None;
                break;
            case CMD_ALIVE:
                // send response to calculate ping
                reply.clear();
                reply << CMD_ALIVE_RESP;
                socket->send(reply);
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
                    if (objectMap.find(id) == objectMap.end() || !objectMap[id])
                    {
                        for(MultiplayerClassListItem* i = multiplayerClassListStart; i; i = i->next)
                        {
                            if (i->name == name)
                            {
                                LOG(INFO) << "Created " << name << " from server replication";
                                MultiplayerObject* obj = i->func();
                                obj->multiplayerObjectId = id;
                                objectMap[id] = obj;

                                while(packet.available())
                                {
                                    int16_t idx;
                                    packet >> idx;
                                    if (idx >= 0 && idx < int16_t(obj->memberReplicationInfo.size()))
                                        (obj->memberReplicationInfo[idx].receiveFunction)(obj->memberReplicationInfo[idx].ptr, packet);
                                    else
                                        LOG(DEBUG) << "Odd index from server replication: " << idx;
                                }
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
                        while(packet.available())
                        {
                            packet >> idx;
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
            case CMD_SERVER_COMMAND:
                {
                    int32_t id;
                    packet >> id;
                    if (objectMap.find(id) != objectMap.end() && objectMap[id])
                    {
                        P<MultiplayerObject> obj = objectMap[id];
                        obj->onReceiveServerCommand(packet);
                    }
                }
                break;
            case CMD_AUDIO_COMM_START:
                {
                    int32_t id = 0;
                    packet >> id;
                    audio_stream_manager.start(id);
                }
                break;
            case CMD_AUDIO_COMM_DATA:
                {
                    int32_t id = 0;
                    packet >> id;
                    const unsigned char* ptr = reinterpret_cast<const unsigned char*>(packet.getData());
                    ptr += sizeof(command_t) + sizeof(int32_t);
                    int32_t size = static_cast<int>(packet.getDataSize()) - sizeof(command_t) - sizeof(int32_t);
                    audio_stream_manager.receivedPacketFromNetwork(id, ptr, size);
                }
                break;
            case CMD_AUDIO_COMM_STOP:
                {
                    int32_t id = 0;
                    packet >> id;
                    audio_stream_manager.stop(id);
                }
                break;
            case CMD_ALIVE:
                // send response to calculate ping
                reply.clear();
                reply << CMD_ALIVE_RESP;
                socket->send(reply);
                break;
            default:
                LOG(ERROR) << "Unknown command from server: " << command;
            }
        case Disconnected:
            break;
        }
    }

    if (socket->getState() == sp::io::network::StreamSocket::State::Closed || no_data_timeout.isExpired())
    {
        if (disconnect_reason == DisconnectReason::None)
            disconnect_reason = socket->getState() != sp::io::network::StreamSocket::State::Closed ? DisconnectReason::TimedOut : DisconnectReason::ClosedByServer;
        socket->close();
        status = Disconnected;
    }
}

void GameClient::sendPacket(sp::io::DataBuffer& packet)
{
    socket->send(packet);
}

void GameClient::sendPassword(string password)
{
    if (status != WaitingForPassword)
        return;

    disconnect_reason = DisconnectReason::BadCredentials;
    sp::io::DataBuffer reply;
    reply << CMD_CLIENT_SEND_AUTH << int32_t(version_number) << password;
    socket->send(reply);
    
    status = Authenticating;
}

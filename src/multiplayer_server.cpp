#include "multiplayer_server.h"
#include "multiplayer_client.h"
#include "multiplayer_internal.h"
#include "engine.h"

#define MULTIPLAYER_COLLECT_DATA_STATS 0

#if MULTIPLAYER_COLLECT_DATA_STATS
sf::Clock multiplayer_stats_dump;
static std::unordered_map<string, int> multiplayer_stats;
#define ADD_MULTIPLAYER_STATS(name, bytes) multiplayer_stats[name] += (bytes)
#else
#define ADD_MULTIPLAYER_STATS(name, bytes) do {} while(0)
#endif

P<GameServer> game_server;

GameServer::GameServer(string server_name, int version_number, int listen_port)
: server_name(server_name), listen_port(listen_port), version_number(version_number), master_server_update_thread(&GameServer::runMasterServerUpdateThread, this)
{
    assert(!game_server);
    assert(!game_client);
    game_server = this;
    lastGameSpeed = engine->getGameSpeed();
    sendDataRate = 0.0;
    sendDataRatePerClient = 0.0;
    boardcastServerDelay = 0.0;
    aliveClock.restart();

    nextObjectId = 1;
    nextclient_id = 1;

    if (listenSocket.listen(static_cast<uint16_t>(listen_port)) != sf::TcpListener::Done)
    {
        LOG(ERROR) << "Failed to listen on TCP port: " << listen_port;
        destroy();
    }
    listenSocket.setBlocking(false);
    new_socket = std::unique_ptr<TcpSocket>(new TcpSocket());
    new_socket->setBlocking(false);
    if (broadcast_listen_socket.bind(static_cast<uint16_t>(listen_port)) != sf::UdpSocket::Done)
    {
        LOG(ERROR) << "Failed to listen on UDP port: " << listen_port;
    }
    broadcast_listen_socket.setBlocking(false);
}

GameServer::~GameServer()
{
    destroy();
    master_server_update_thread.wait();
}

void GameServer::connectToProxy(sf::IpAddress address, int port)
{
    auto socket = std::unique_ptr<TcpSocket>(new TcpSocket());
    LOG(INFO) << "Connecting to proxy: " << address.toString();
    if (socket->connect(address, static_cast<uint16_t>(port)) != sf::Socket::Status::Done)
    {
        LOG(ERROR) << "Failed to connect to proxy";
        return;
    }

    ClientInfo info;
    info.socket = std::move(socket);
    info.socket->setBlocking(false);
    info.client_id = nextclient_id;
    info.receive_state = CRS_Auth;
    nextclient_id++;
    {
        sf::Packet packet;
        packet << CMD_SERVER_CONNECT_TO_PROXY;
        info.socket->send(packet);
    }
    {
        sf::Packet packet;
        packet << CMD_REQUEST_AUTH << int32_t(version_number) << bool(server_password != "");
        info.socket->send(packet);
    }
    LOG(INFO) << "New proxy connection: " << info.client_id << " waiting for authentication";
    clientList.push_back(std::move(info));
}

void GameServer::destroy()
{
    clientList.clear();
    objectMap.clear();

    listenSocket.close();
    broadcast_listen_socket.unbind();

    Updatable::destroy();
}

P<MultiplayerObject> GameServer::getObjectById(int32_t id)
{
    if (objectMap.find(id) != objectMap.end())
        return objectMap[id];
    return NULL;
}

void GameServer::update(float /*gameDelta*/)
{
    sf::Clock update_run_time_clock;    //Clock used to measure how much time this update cycle is costing us.
    
    //Calculate our own delta, as we want wall-time delta, the gameDelta can be modified by the current game speed (could even be 0 on pause)
    float delta = updateTimeClock.getElapsedTime().asSeconds();
    updateTimeClock.restart();

    sendDataCounter = 0;
    sendDataCounterPerClient = 0;

    if (lastGameSpeed != engine->getGameSpeed())
    {
        lastGameSpeed = engine->getGameSpeed();
        sf::Packet packet;
        packet << CMD_SET_GAME_SPEED << lastGameSpeed;
        sendAll(packet);
    }

    std::vector<int32_t> delList;
    for(std::unordered_map<int32_t, P<MultiplayerObject> >::iterator i=objectMap.begin(); i != objectMap.end(); i++)
    {
        int id = i->first;
        P<MultiplayerObject> obj = i->second;
        if (obj)
        {
            if (!obj->replicated)
            {
                obj->replicated = true;

                sf::Packet packet;
                generateCreatePacketFor(obj, packet);
                //Call the isChanged function for each replication info, so the prev_data is updated.
                for(unsigned int n=0; n<obj->memberReplicationInfo.size(); n++)
                    obj->memberReplicationInfo[n].isChangedFunction(obj->memberReplicationInfo[n].ptr, &obj->memberReplicationInfo[n].prev_data);
                sendAll(packet);
                ADD_MULTIPLAYER_STATS(obj->multiplayerClassIdentifier + "::CREATE", packet.getDataSize());
            }
            sf::Packet packet;
            packet << CMD_UPDATE_VALUE;
            packet << int32_t(obj->multiplayerObjectId);
#if MULTIPLAYER_COLLECT_DATA_STATS
            int overhead = packet.getDataSize();
#endif
            int cnt = 0;
            for(unsigned int n=0; n<obj->memberReplicationInfo.size(); n++)
            {
                if (obj->memberReplicationInfo[n].update_timeout > 0.0)
                {
                    obj->memberReplicationInfo[n].update_timeout -= delta;
                }else{
                    if ((obj->memberReplicationInfo[n].isChangedFunction)(obj->memberReplicationInfo[n].ptr, &obj->memberReplicationInfo[n].prev_data))
                    {
#if MULTIPLAYER_COLLECT_DATA_STATS
                        int packet_size = packet.getDataSize();
#endif
                        packet << int16_t(n);
                        (obj->memberReplicationInfo[n].sendFunction)(obj->memberReplicationInfo[n].ptr, packet);
                        cnt++;
                        ADD_MULTIPLAYER_STATS(obj->multiplayerClassIdentifier + "::" + obj->memberReplicationInfo[n].name, packet.getDataSize() - packet_size);

                        obj->memberReplicationInfo[n].update_timeout = obj->memberReplicationInfo[n].update_delay;
                    }
                }
            }
            if (cnt > 0)
            {
                sendAll(packet);
                ADD_MULTIPLAYER_STATS(obj->multiplayerClassIdentifier + "::OVERHEAD", overhead);
            }
        }else{
            delList.push_back(id);
        }
    }
    for(unsigned int n=0; n<delList.size(); n++)
    {
        sf::Packet packet;
        generateDeletePacketFor(delList[n], packet);
        sendAll(packet);
        ADD_MULTIPLAYER_STATS("???::DELETE", packet.getDataSize());
        objectMap.erase(delList[n]);
    }

    handleBroadcastUDPSocket(delta);

    if (listenSocket.accept(*new_socket)==sf::Socket::Status::Done)
    {
        ClientInfo info;
        info.socket = std::move(new_socket);
        new_socket = std::unique_ptr<TcpSocket>(new TcpSocket());
        new_socket->setBlocking(false);
        info.client_id = nextclient_id;
        info.receive_state = CRS_Auth;
        nextclient_id++;
        {
            sf::Packet packet;
            packet << CMD_REQUEST_AUTH << int32_t(version_number) << bool(server_password != "");
            info.socket->send(packet);
        }
        LOG(INFO) << "New connection: " << info.client_id << " waiting for authentication";
        clientList.push_back(std::move(info));
    }

    for(unsigned int n=0; n<clientList.size(); n++)
    {
        clientList[n].socket->update();
        sf::Packet packet;
        sf::TcpSocket::Status status{ sf::TcpSocket::Error };
        while(clientList[n].socket && (status = clientList[n].socket->receive(packet)) == sf::TcpSocket::Done)
        {
            switch(clientList[n].receive_state)
            {
            case CRS_Auth:
                {
                    command_t command;
                    packet >> command;
                    switch(command)
                    {
                    case CMD_SERVER_CONNECT_TO_PROXY:
                        clientList[n].socket->disconnect();
                        clientList[n].socket = NULL;
                        break;
                    case CMD_REQUEST_AUTH:
                        break;
                    case CMD_CLIENT_SEND_AUTH:
                        {
                            int32_t client_version;
                            string client_password;
                            packet >> client_version >> client_password;

                            if (version_number == client_version || version_number == 0 || client_version == 0)
                            {
                                if (server_password == "" || client_password == server_password)
                                {
                                    clientList[n].receive_state = CRS_Main;
                                    handleNewClient(clientList[n]);
                                }else{
                                    //Wrong password, send a new auth request so the client knows the password was not accepted.
                                    sf::Packet auth_request_packet;
                                    auth_request_packet << CMD_REQUEST_AUTH << int32_t(version_number) << bool(server_password != "");
                                    clientList[n].socket->send(auth_request_packet);
                                }
                            }else{
                                LOG(ERROR) << n << ":Client version mismatch: " << version_number << " != " << client_version;
                                clientList[n].socket->disconnect();
                                clientList[n].socket = NULL;
                            }
                            break;
                        }
                        break;
                    case CMD_ALIVE_RESP:
                        {
                            clientList[n].ping = clientList[n].round_trip_time.getElapsedTime().asMilliseconds();
                        }
                        break;
                    default:
                        LOG(ERROR) << "Unknown command from client while authenticating: " << command;
                        clientList[n].socket->disconnect();
                        clientList[n].socket = NULL;
                        break;
                    }
                }
                break;
            case CRS_Main:
                {
                    command_t command;
                    packet >> command;
                    switch(command)
                    {
                    case CMD_NEW_PROXY_CLIENT:
                        {
                            int32_t temp_id = 0;
                            packet >> temp_id;
                            handleNewProxy(clientList[n], temp_id);
                        }
                        break;
                    case CMD_DEL_PROXY_CLIENT:
                        {
                            int32_t client_id = 0;
                            packet >> client_id;
                            for(auto id : clientList[n].proxy_ids)
                            {
                                if (id == client_id)
                                {
                                    onDisconnectClient(client_id);
                                }
                            }
                            clientList[n].proxy_ids.erase(std::remove_if(clientList[n].proxy_ids.begin(), clientList[n].proxy_ids.end(), [client_id](int32_t id) {return id == client_id;}), clientList[n].proxy_ids.end());
                        }
                        break;
                    case CMD_CLIENT_COMMAND:
                        packet >> clientList[n].command_object_id;
                        clientList[n].command_client_id = clientList[n].client_id;
                        clientList[n].receive_state = CRS_Command;
                        break;
                    case CMD_PROXY_CLIENT_COMMAND:
                        {
                            int32_t client_id = 0;
                            packet >> clientList[n].command_object_id >> client_id;
                            clientList[n].command_client_id = clientList[n].client_id;
                            for(auto id : clientList[n].proxy_ids)
                                if (id == client_id)
                                    clientList[n].command_client_id = client_id;
                            clientList[n].receive_state = CRS_Command;
                        }
                        break;
                    case CMD_AUDIO_COMM_START:
                        {
                            int32_t target_identifier = 0;
                            int32_t client_id = 0;
                            packet >> client_id >> target_identifier;
                            if (client_id == clientList[n].client_id)
                            {
                                startAudio(client_id, target_identifier);
                            }
                            else
                            {
                                for(auto id : clientList[n].proxy_ids)
                                    if (id == client_id)
                                        startAudio(client_id, target_identifier);
                            }
                        }
                        break;
                    case CMD_AUDIO_COMM_DATA:
                        if (packet.getDataSize() > sizeof(int32_t) + sizeof(command_t))
                        {
                            int32_t client_id;
                            packet >> client_id;

                            const unsigned char* ptr = reinterpret_cast<const unsigned char*>(packet.getData());
                            ptr += sizeof(int32_t) + sizeof(command_t);
                            if (client_id == clientList[n].client_id)
                            {
                                gotAudioPacket(client_id, ptr, static_cast<int>(packet.getDataSize()) - sizeof(int32_t) - sizeof(command_t));
                            }
                            else
                            {
                                for(auto id : clientList[n].proxy_ids)
                                    if (id == client_id)
                                        gotAudioPacket(client_id, ptr, static_cast<int>(packet.getDataSize()) - sizeof(int32_t) - sizeof(command_t));
                            }
                        }
                        break;
                    case CMD_AUDIO_COMM_STOP:
                        {
                            int32_t client_id;
                            packet >> client_id;
                            if (client_id == clientList[n].client_id)
                            {
                                stopAudio(client_id);
                            }
                            else
                            {
                                for(auto id : clientList[n].proxy_ids)
                                    if (id == client_id)
                                        stopAudio(client_id);
                            }
                        }
                        break;
                    case CMD_ALIVE_RESP:
                        {
                            clientList[n].ping = clientList[n].round_trip_time.getElapsedTime().asMilliseconds();
                        }
                    break;
                    default:
                        LOG(ERROR) << "Unknown command from client: " << command;
                    }
                }
                break;
            case CRS_Command:
                if (objectMap.find(clientList[n].command_object_id) != objectMap.end() && objectMap[clientList[n].command_object_id])
                    objectMap[clientList[n].command_object_id]->onReceiveClientCommand(clientList[n].command_client_id, packet);
                clientList[n].receive_state = CRS_Main;
                break;
            }
        }
        if (status == sf::TcpSocket::Disconnected || clientList[n].socket == NULL)
        {
            if (clientList[n].socket)
            {
                for(auto id : clientList[n].proxy_ids)
                    onDisconnectClient(id);
                onDisconnectClient(clientList[n].client_id);
            }
            clientList.erase(clientList.begin() + n);
            n--;
        }
    }

    if (aliveClock.getElapsedTime().asSeconds() > 10.0)
    {
        aliveClock.restart();

        keepAliveAll();
    }

    float dataPerSecond = float(sendDataCounter) / delta;
    sendDataRate = sendDataRate * (1.f - delta) + dataPerSecond * delta;
    dataPerSecond = float(sendDataCounterPerClient) / delta;
    sendDataRatePerClient = sendDataRatePerClient * (1.f - delta) + dataPerSecond * delta;

#if MULTIPLAYER_COLLECT_DATA_STATS
    if (multiplayer_stats_dump.getElapsedTime().asSeconds() > 1.0)
    {
        multiplayer_stats_dump.restart();

        int total = 0;
        for(std::unordered_map<string, int >::iterator i=multiplayer_stats.begin(); i != multiplayer_stats.end(); i++)
            total += i->second;
        printf("---------------------------------Total: %d\n", total);
        for(std::unordered_map<string, int >::iterator i=multiplayer_stats.begin(); i != multiplayer_stats.end(); i++)
        {
            printf("%60s: %d (%d%%)\n", i->first.c_str(), i->second, i->second * 100 / total);
        }
        multiplayer_stats.clear();
    }
#endif
    update_run_time = update_run_time_clock.getElapsedTime().asSeconds();
}

void GameServer::handleNewClient(ClientInfo& info)
{
    {
        sf::Packet packet;
        packet << CMD_SET_CLIENT_ID << info.client_id;
        info.socket->send(packet);
    }
    {
        sf::Packet packet;
        packet << CMD_SET_GAME_SPEED << lastGameSpeed;
        info.socket->send(packet);
    }

    onNewClient(info.client_id);

    //On a new client, first create all the already existing objects. And update all the values.
    for(std::unordered_map<int32_t, P<MultiplayerObject> >::iterator i=objectMap.begin(); i != objectMap.end(); i++)
    {
        P<MultiplayerObject> obj = i->second;
        if (obj && obj->replicated)
        {
            sf::Packet packet;
            generateCreatePacketFor(obj, packet);
            sendDataCounter += packet.getDataSize();
            info.socket->send(packet);
        }
    }
}

void GameServer::handleNewProxy(ClientInfo& info, int32_t temp_id)
{
    info.proxy_ids.push_back(nextclient_id);
    {
        sf::Packet packet;
        packet << CMD_SET_PROXY_CLIENT_ID << temp_id << nextclient_id;
        info.socket->send(packet);
        nextclient_id++;
    }
    {
        sf::Packet packet;
        packet << CMD_SET_GAME_SPEED << lastGameSpeed;
        info.socket->send(packet);
    }

    onNewClient(info.proxy_ids.back());

    //On a new client, first create all the already existing objects. And update all the values.
    for(std::unordered_map<int32_t, P<MultiplayerObject> >::iterator i=objectMap.begin(); i != objectMap.end(); i++)
    {
        P<MultiplayerObject> obj = i->second;
        if (obj && obj->replicated)
        {
            sf::Packet packet;
            generateCreatePacketFor(obj, packet);
            sendDataCounter += packet.getDataSize();
            info.socket->send(packet);
        }
    }
}


void GameServer::handleBroadcastUDPSocket(float delta)
{
    sf::IpAddress recvAddress;
    unsigned short recvPort;
    sf::Packet recvPacket;
    if (broadcast_listen_socket.receive(recvPacket, recvAddress, recvPort) == sf::Socket::Status::Done)
    {
        //We do not care about what we received. Reply that we live!
        sf::Packet sendPacket;
        sendPacket << int32_t(multiplayerVerficationNumber) << int32_t(version_number) << server_name;
        broadcast_listen_socket.send(sendPacket, recvAddress, recvPort);
    }
    if (boardcastServerDelay > 0.0)
    {
        boardcastServerDelay -= delta;
    }else{
        boardcastServerDelay = 5.0;

        sf::Packet sendPacket;
        sendPacket << int32_t(multiplayerVerficationNumber) << int32_t(version_number) << server_name;
        UDPbroadcastPacket(broadcast_listen_socket, sendPacket, broadcast_listen_socket.getLocalPort() + 1);
    }
}

void GameServer::registerObject(P<MultiplayerObject> obj)
{
    //Note, at this point in time, the pointed object is only of the MultiplayerObject class.
    // This due to the fact that in C++ does not "is" it's final sub-class till construction is completed.
    obj->multiplayerObjectId = nextObjectId;
    obj->replicated = false;
    nextObjectId++;

    objectMap[obj->multiplayerObjectId] = obj;
}

void GameServer::setPassword(string password)
{
    server_password = password;
}

void GameServer::generateCreatePacketFor(P<MultiplayerObject> obj, sf::Packet& packet)
{
    packet << CMD_CREATE << obj->multiplayerObjectId << obj->multiplayerClassIdentifier;

    for(unsigned int n=0; n<obj->memberReplicationInfo.size(); n++)
    {
        packet << int16_t(n);
        (obj->memberReplicationInfo[n].sendFunction)(obj->memberReplicationInfo[n].ptr, packet);
    }
}

void GameServer::generateDeletePacketFor(int32_t id, sf::Packet& packet)
{
    packet << CMD_DELETE << id;
}

void GameServer::broadcastServerCommandFromObject(int32_t id, sf::Packet& packet)
{
    sf::Packet p;
    p << CMD_SERVER_COMMAND << id;
    p.append(packet.getData(), packet.getDataSize());
    sendAll(p);
}

void GameServer::keepAliveAll()
{
    sf::Packet packet;
    packet << CMD_ALIVE;
    sendDataCounterPerClient += packet.getDataSize();
    for(auto& client : clientList)
    {
        if (client.socket)
        {
            client.round_trip_time.restart();
            client.socket->send(packet);
        }
    }
}

void GameServer::sendAll(sf::Packet& packet)
{
    sendDataCounterPerClient += packet.getDataSize();
    for(auto& client : clientList)
    {
        if (client.receive_state != CRS_Auth && client.socket)
            client.socket->send(packet);
    }
}

void GameServer::registerOnMasterServer(string master_url)
{
    this->master_server_url = master_url;
    master_server_update_thread.launch();
}

void GameServer::stopMasterServerRegistry()
{
    this->master_server_url = "";
    master_server_update_thread.wait();
}

void GameServer::runMasterServerUpdateThread()
{
    if (!master_server_url.startswith("http://"))
    {
        LOG(ERROR) << "Master server URL " << master_server_url << " does not start with \"http://\"";
        return;
    }
    string hostname = master_server_url.substr(7);
    int path_start = hostname.find("/");
    if (path_start < 0)
    {
        LOG(ERROR) << "Master server URL " << master_server_url << " does not have a URI after the hostname";
        return;
    }
    int port = 80;
    int port_start = hostname.find(":");
    string uri = hostname.substr(path_start + 1);
    if (port_start >= 0)
    {
        // If a port is attached to the hostname, parse it out.
        // No validation is performed.
        port = hostname.substr(port_start + 1, path_start).toInt();
        hostname = hostname.substr(0, port_start);
    }else{
        hostname = hostname.substr(0, path_start);
    }
    
    LOG(INFO) << "Registering at master server " << master_server_url;
    
    sf::Http http(hostname, static_cast<uint16_t>(port));
    while(!isDestroyed() && master_server_url != "")
    {
        sf::Http::Request request(uri, sf::Http::Request::Post);
        request.setBody("port=" + string(listen_port) + "&name=" + server_name + "&version=" + string(version_number));
        
        sf::Http::Response response = http.sendRequest(request, sf::seconds(10.0f));
        if (response.getStatus() != sf::Http::Response::Ok)
        {
            LOG(WARNING) << "Failed to register at master server " << master_server_url << " (status " << response.getStatus() << ")";
        }else if (response.getBody() != "OK")
        {
            LOG(WARNING) << "Master server " << master_server_url << " reports error on registering: " << response.getBody();
        }
        
        for(int n=0;n<60 && !isDestroyed() && master_server_url != "";n++)
            sf::sleep(sf::seconds(1.0f));
    }
}

void GameServer::startAudio(int32_t client_id, int32_t target_identifier)
{
    voice_targets[client_id] = onVoiceChat(client_id, target_identifier);

    sf::Packet audio_packet;
    audio_packet << CMD_AUDIO_COMM_START << client_id;
    sendAudioPacketFrom(client_id, audio_packet);

    if (client_id != 0)
        audio_stream_manager.start(client_id);
}

void GameServer::gotAudioPacket(int32_t client_id, const unsigned char* packet, int packet_size)
{
    sf::Packet audio_packet;
    audio_packet << CMD_AUDIO_COMM_DATA << client_id;
    audio_packet.append(packet, packet_size);
    sendAudioPacketFrom(client_id, audio_packet);

    if (client_id != 0)
        audio_stream_manager.receivedPacketFromNetwork(client_id, packet, packet_size);
}

void GameServer::stopAudio(int32_t client_id)
{
    sf::Packet audio_packet;
    audio_packet << CMD_AUDIO_COMM_STOP << client_id;
    sendAudioPacketFrom(client_id, audio_packet);

    voice_targets.erase(client_id);

    if (client_id != 0)
        audio_stream_manager.stop(client_id);
}

void GameServer::sendAudioPacketFrom(int32_t client_id, sf::Packet& packet)
{
    auto it = voice_targets.find(client_id);
    if (it == voice_targets.end())
        return;
    auto& ids = it->second;

    for(auto& client : clientList)
    {
        if (client.receive_state != CRS_Auth && client.socket)
        {
            bool send = ids.find(client.client_id) != ids.end();
            if (client.proxy_ids.size() > 0)
            {
                sf::Packet target_packet;
                target_packet << CMD_PROXY_TO_CLIENTS;
                for(auto id : client.proxy_ids)
                {
                    if (ids.find(id) != ids.end())
                    {
                        target_packet << id;
                        send = true;
                    }
                }
            }
            if (send)
            {
                client.socket->send(packet);
            }
        }
    }
}

std::unordered_set<int32_t> GameServer::onVoiceChat(int32_t client_id, int32_t /*target_identifier*/)
{
    //Default, target all clients
    std::unordered_set<int32_t> result;
    if (client_id != 0)
        result.insert(0);
    for(auto& client : clientList)
    {
        if (client.receive_state != CRS_Auth)
        {
            if (client_id != client.client_id)
                result.insert(client.client_id);
            for(auto id : client.proxy_ids)
                if (client_id != id)
                    result.insert(id);
        }
    }
    return result;
}

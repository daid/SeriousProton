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

    if (listenSocket.listen(listen_port) != sf::TcpListener::Done)
    {
        LOG(ERROR) << "Failed to listen on TCP port: " << listen_port;
        destroy();
    }
    if (broadcast_listen_socket.bind(listen_port) != sf::UdpSocket::Done)
    {
        LOG(ERROR) << "Failed to listen on UDP port: " << listen_port;
    }
    selector.add(listenSocket);
    selector.add(broadcast_listen_socket);
}

GameServer::~GameServer()
{
    destroy();
    master_server_update_thread.wait();
}

void GameServer::destroy()
{
    for(ClientInfo& info : clientList)
        delete info.socket;
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

void GameServer::update(float gameDelta)
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
                genenerateCreatePacketFor(obj, packet);
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
        genenerateDeletePacketFor(delList[n], packet);
        sendAll(packet);
        ADD_MULTIPLAYER_STATS("???::DELETE", packet.getDataSize());
        objectMap.erase(delList[n]);
    }

    selector.wait(sf::microseconds(1));//Seems to delay 1ms on Windows. Not ideal, but fast enough. (other option is using threads, which I rather avoid)
    handleBroadcastUDPSocket(delta);

    if (selector.isReady(listenSocket))
    {
        ClientInfo info;
        info.socket = new TcpSocket();
        info.socket->setBlocking(false);
        info.client_id = nextclient_id;
        info.receive_state = CRS_Auth;
        nextclient_id++;
        listenSocket.accept(*info.socket);
        clientList.push_back(info);
        selector.add(*info.socket);
        {
            sf::Packet packet;
            packet << CMD_REQUEST_AUTH << int32_t(version_number) << bool(server_password != "");
            info.socket->send(packet);
        }
        LOG(INFO) << "New connection: " << info.client_id << " waiting for authentication";
    }

    for(unsigned int n=0; n<clientList.size(); n++)
    {
        clientList[n].socket->update();
        if (selector.isReady(*clientList[n].socket))
        {
            sf::Packet packet;
            sf::TcpSocket::Status status;
            while((status = clientList[n].socket->receive(packet)) == sf::TcpSocket::Done)
            {
                switch(clientList[n].receive_state)
                {
                case CRS_Auth:
                    {
                        command_t command;
                        packet >> command;
                        switch(command)
                        {
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
                                        sf::Packet packet;
                                        packet << CMD_REQUEST_AUTH << int32_t(version_number) << bool(server_password != "");
                                        clientList[n].socket->send(packet);
                                    }
                                }else{
                                    LOG(ERROR) << n << ":Client version mismatch: " << version_number << " != " << client_version;
                                    clientList[n].socket->disconnect();
                                }
                                break;
                            }
                            break;
                        default:
                            LOG(ERROR) << "Unknown command from client while authenticating: " << command;
                            clientList[n].socket->disconnect();
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
                        case CMD_CLIENT_COMMAND:
                            packet >> clientList[n].command_object_id;
                            clientList[n].receive_state = CRS_Command;
                            break;
                        case CMD_CLIENT_AUDIO_COMM:
                            {
                                int32_t target_identifier;
                                uint32_t sample_count;
                                std::vector<int16_t> samples;
                                packet >> target_identifier;
                                packet >> sample_count;
                                samples.reserve(sample_count);
                                for(unsigned int n=0; n<sample_count; n++)
                                {
                                    int16_t sample;
                                    packet >> sample;
                                    samples.push_back(sample);
                                }
                                gotAudioPacket(n, target_identifier, samples);
                            }
                        default:
                            LOG(ERROR) << "Unknown command from client: " << command;
                        }
                    }
                    break;
                case CRS_Command:
                    if (objectMap.find(clientList[n].command_object_id) != objectMap.end() && objectMap[clientList[n].command_object_id])
                        objectMap[clientList[n].command_object_id]->onReceiveClientCommand(clientList[n].client_id, packet);
                    clientList[n].receive_state = CRS_Main;
                    break;
                }
            }
            if (status == sf::TcpSocket::Disconnected)
            {
                onDisconnectClient(clientList[n].client_id);
                selector.remove(*clientList[n].socket);
                delete clientList[n].socket;
                clientList.erase(clientList.begin() + n);
                n--;
            }
        }
    }

    if (aliveClock.getElapsedTime().asSeconds() > 10.0)
    {
        aliveClock.restart();

        sf::Packet packet;
        packet << CMD_ALIVE;
        sendAll(packet);
    }

    float dataPerSecond = float(sendDataCounter) / delta;
    sendDataRate = sendDataRate * (1.0 - delta) + dataPerSecond * delta;
    dataPerSecond = float(sendDataCounterPerClient) / delta;
    sendDataRatePerClient = sendDataRatePerClient * (1.0 - delta) + dataPerSecond * delta;

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
            genenerateCreatePacketFor(obj, packet);
            sendDataCounter += packet.getDataSize();
            info.socket->send(packet);
        }
    }
}

void GameServer::handleBroadcastUDPSocket(float delta)
{
    if (selector.isReady(broadcast_listen_socket))
    {
        sf::IpAddress recvAddress;
        unsigned short recvPort;
        sf::Packet recvPacket;
        broadcast_listen_socket.receive(recvPacket, recvAddress, recvPort);

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

void GameServer::genenerateCreatePacketFor(P<MultiplayerObject> obj, sf::Packet& packet)
{
    packet << CMD_CREATE << obj->multiplayerObjectId << obj->multiplayerClassIdentifier;

    for(unsigned int n=0; n<obj->memberReplicationInfo.size(); n++)
    {
        packet << int16_t(n);
        (obj->memberReplicationInfo[n].sendFunction)(obj->memberReplicationInfo[n].ptr, packet);
    }
}

void GameServer::genenerateDeletePacketFor(int32_t id, sf::Packet& packet)
{
    packet << CMD_DELETE << id;
}

void GameServer::sendAll(sf::Packet& packet)
{
    sendDataCounterPerClient += packet.getDataSize();
    for(unsigned int n=0; n<clientList.size(); n++)
    {
        if (clientList[n].receive_state != CRS_Auth)
            clientList[n].socket->send(packet);
    }
}

void GameServer::registerOnMasterServer(string master_server_url)
{
    this->master_server_url = master_server_url;
    master_server_update_thread.launch();
}

void GameServer::stopMasterServerRegistry()
{
    this->master_server_url = "";
}

void GameServer::runMasterServerUpdateThread()
{
    if (!master_server_url.startswith("http://"))
    {
        LOG(ERROR) << "Master server URL does not start with \"http://\"";
        return;
    }
    string hostname = master_server_url.substr(7);
    int path_start = hostname.find("/");
    if (path_start < 0)
    {
        LOG(ERROR) << "Master server URL has no uri after hostname";
        return;
    }
    string uri = hostname.substr(path_start + 1);
    hostname = hostname.substr(0, path_start);
    
    LOG(INFO) << "Registering at master server";
    
    sf::Http http(hostname);
    while(!isDestroyed() && master_server_url != "")
    {
        sf::Http::Request request(uri, sf::Http::Request::Post);
        request.setBody("port=" + string(listen_port) + "&name=" + server_name + "&version=" + string(version_number));
        
        sf::Http::Response response = http.sendRequest(request, sf::seconds(10.0f));
        if (response.getStatus() != sf::Http::Response::Ok)
        {
            LOG(WARNING) << "Failed to register at master server (" << response.getStatus() << ")";
        }else if (response.getBody() != "OK")
        {
            LOG(WARNING) << "Master server reports error on registering: " << response.getBody();
        }
        
        for(int n=0;n<60 && !isDestroyed() && master_server_url != "";n++)
            sf::sleep(sf::seconds(1.0f));
    }
}

void GameServer::gotAudioPacket(int32_t client_id, int32_t target_identifier, std::vector<int16_t>& audio_data)
{
}

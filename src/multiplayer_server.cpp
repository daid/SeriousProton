#include "multiplayer_server.h"
#include "multiplayer_client.h"
#include "multiplayer_internal.h"
#include "engine.h"

#define MULTIPLAYER_COLLECT_DATA_STATS 0

#if MULTIPLAYER_COLLECT_DATA_STATS
sf::Clock multiplayer_stats_dump;
static std::map<string, int> multiplayer_stats;
#define ADD_MULTIPLAYER_STATS(name, bytes) multiplayer_stats[name] += (bytes)
#else
#define ADD_MULTIPLAYER_STATS(name, bytes) do {} while(0)
#endif

P<GameServer> game_server;

GameServer::GameServer(string serverName, int versionNumber, int listenPort)
: serverName(serverName), versionNumber(versionNumber)
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

    if (listenSocket.listen(listenPort) != sf::TcpListener::Done)
    {
        LOG(ERROR) << "Failed to listen on TCP port: " << listenPort;
    }
    if (broadcastListenSocket.bind(listenPort) != sf::UdpSocket::Done)
    {
        LOG(ERROR) << "Failed to listen on UDP port: " << listenPort;
    }
    selector.add(listenSocket);
    selector.add(broadcastListenSocket);
}

P<MultiplayerObject> GameServer::getObjectById(int32_t id)
{
    if (objectMap.find(id) != objectMap.end())
        return objectMap[id];
    return NULL;
}

void GameServer::update(float gameDelta)
{
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
    for(std::map<int32_t, P<MultiplayerObject> >::iterator i=objectMap.begin(); i != objectMap.end(); i++)
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
    if (selector.isReady(broadcastListenSocket))
    {
        sf::IpAddress recvAddress;
        unsigned short recvPort;
        sf::Packet recvPacket;
        broadcastListenSocket.receive(recvPacket, recvAddress, recvPort);

        //We do not care about what we received. Reply that we live!
        sf::Packet sendPacket;
        sendPacket << int32_t(multiplayerVerficationNumber) << int32_t(versionNumber) << serverName;
        broadcastListenSocket.send(sendPacket, recvAddress, recvPort);
    }
    if (boardcastServerDelay > 0.0)
    {
        boardcastServerDelay -= delta;
    }else{
        boardcastServerDelay = 5.0;

        sf::Packet sendPacket;
        sendPacket << int32_t(multiplayerVerficationNumber) << int32_t(versionNumber) << serverName;
        UDPbroadcastPacket(broadcastListenSocket, sendPacket, broadcastListenSocket.getLocalPort() + 1);
    }

    if (selector.isReady(listenSocket))
    {
        ClientInfo info;
        info.socket = new TcpSocket();
        info.socket->setBlocking(false);
        info.client_id = nextclient_id;
        info.receiveState = CRS_Main;
        nextclient_id++;
        listenSocket.accept(*info.socket);
        clientList.push_back(info);
        selector.add(*info.socket);
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
        for(std::map<int32_t, P<MultiplayerObject> >::iterator i=objectMap.begin(); i != objectMap.end(); i++)
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

    for(unsigned int n=0; n<clientList.size(); n++)
    {
        clientList[n].socket->update();
        if (selector.isReady(*clientList[n].socket))
        {
            sf::Packet packet;
            sf::TcpSocket::Status status;
            while((status = clientList[n].socket->receive(packet)) == sf::TcpSocket::Done)
            {
                switch(clientList[n].receiveState)
                {
                case CRS_Main:
                    {
                        command_t command;
                        packet >> command;
                        switch(command)
                        {
                        case CMD_CLIENT_COMMAND:
                            packet >> clientList[n].command_object_id;
                            clientList[n].receiveState = CRS_Command;
                            break;
                        default:
                            LOG(ERROR) << "Unknown command from client: " << command;
                        }
                    }
                    break;
                case CRS_Command:
                    if (objectMap.find(clientList[n].command_object_id) != objectMap.end() && objectMap[clientList[n].command_object_id])
                        objectMap[clientList[n].command_object_id]->onReceiveClientCommand(clientList[n].client_id, packet);
                    clientList[n].receiveState = CRS_Main;
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
        for(std::map<string, int >::iterator i=multiplayer_stats.begin(); i != multiplayer_stats.end(); i++)
            total += i->second;
        printf("---------------------------------Total: %d\n", total);
        for(std::map<string, int >::iterator i=multiplayer_stats.begin(); i != multiplayer_stats.end(); i++)
        {
            printf("%60s: %d (%d%%)\n", i->first.c_str(), i->second, i->second * 100 / total);
        }
        multiplayer_stats.clear();
    }
#endif
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
        clientList[n].socket->send(packet);
}

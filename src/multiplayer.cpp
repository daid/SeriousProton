#include "multiplayer.h"
#include "Collisionable.h"
#include "engine.h"

static const int16_t CMD_CREATE = 0x0001;
static const int16_t CMD_UPDATE_VALUE = 0x0002;
static const int16_t CMD_DELETE = 0x0003;
static const int16_t CMD_SET_CLIENT_ID = 0x0004;
static const int16_t CMD_SET_GAME_SPEED = 0x0005;
static const int16_t CMD_CLIENT_COMMAND = 0x0006;

P<GameServer> gameServer;
P<GameClient> gameClient;
MultiplayerClassListItem* multiplayerClassListStart;

GameServer::GameServer(string serverName, int versionNumber, int listenPort)
: serverName(serverName), versionNumber(versionNumber)
{
    assert(!gameServer);
    assert(!gameClient);
    gameServer = this;
    lastGameSpeed = engine->getGameSpeed();

    nextObjectId = 1;
    nextClientId = 1;
    
    if (listenSocket.listen(listenPort) != sf::TcpListener::Done)
    {
        printf("Failed to listen on TCP port: %d\n", listenPort);
    }
    if (broadcastListenSocket.bind(listenPort) != sf::UdpSocket::Done)
    {
        printf("Failed to listen on UDP port: %d\n", listenPort);
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
                sendAll(packet);
            }
            for(unsigned int n=0; n<obj->memberReplicationInfo.size(); n++)
            {
                if (obj->memberReplicationInfo[n].update_timeout > 0.0)
                {
                    obj->memberReplicationInfo[n].update_timeout -= delta;
                }else{
                    if ((obj->memberReplicationInfo[n].isChangedFunction)(obj->memberReplicationInfo[n].ptr, &obj->memberReplicationInfo[n].prev_data))
                    {
                        sf::Packet packet;
                        packet << CMD_UPDATE_VALUE;
                        packet << int32_t(obj->multiplayerObjectId);
                        packet << int16_t(n);
                        (obj->memberReplicationInfo[n].sendFunction)(obj->memberReplicationInfo[n].ptr, packet);
                        sendAll(packet);
                        
                        obj->memberReplicationInfo[n].update_timeout = obj->memberReplicationInfo[n].update_delay;
                    }
                }
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
        sendPacket << int32_t(multiplayerMagicNumber) << int32_t(versionNumber) << serverName;
        broadcastListenSocket.send(sendPacket, recvAddress, recvPort);
    }
    
    if (selector.isReady(listenSocket))
    {
        ClientInfo info;
        info.socket = new sf::TcpSocket();
        info.clientId = nextClientId;
        nextClientId++;
        listenSocket.accept(*info.socket);
        clientList.push_back(info);
        selector.add(*info.socket);
        {
            sf::Packet packet;
            packet << CMD_SET_CLIENT_ID << info.clientId;
            info.socket->send(packet);
        }
        {
            sf::Packet packet;
            packet << CMD_SET_GAME_SPEED << lastGameSpeed;
            info.socket->send(packet);
        }
        
        onNewClient(info.clientId);
        
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
        if (selector.isReady(*clientList[n].socket))
        {
            sf::Packet packet;
            if (clientList[n].socket->receive(packet) == sf::TcpSocket::Done)
            {
                int16_t command;
                packet >> command;
                switch(command)
                {
                case CMD_CLIENT_COMMAND:
                    {
                        int32_t id;
                        packet >> id;
                        if (objectMap.find(id) != objectMap.end() && objectMap[id])
                        {
                            clientList[n].socket->receive(packet);
                            objectMap[id]->onReceiveCommand(clientList[n].clientId, packet);
                        }
                    }
                    break;
                default:
                    printf("Unknown packet from client: %d\n", command);
                }
            }else{
                onDisconnectClient(clientList[n].clientId);
                selector.remove(*clientList[n].socket);
                delete clientList[n].socket;
                clientList.erase(clientList.begin() + n);
                n--;
            }
        }
    }
    
    float dataPerSecond = float(sendDataCounter) / delta;
    sendDataRate = sendDataRate * (1.0 - delta) + dataPerSecond * delta;
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
    for(unsigned int n=0; n<clientList.size(); n++)
    {
        sendDataCounter += packet.getDataSize();
        clientList[n].socket->send(packet);
    }
}

GameClient::GameClient(sf::IpAddress server, int portNr)
{
    assert(!gameServer);
    assert(!gameClient);
    
    clientId = -1;
    gameClient = this;

    if (socket.connect(server, portNr) != sf::TcpSocket::Done)
    {
        destroy();
    }
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

    sf::Packet packet;
    while(socket.receive(packet) == sf::TcpSocket::Done)
    {
        int16_t command;
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
                        printf("Created %s from server replication\n", name.c_str());
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
                packet >> id >> idx;
                if (objectMap.find(id) != objectMap.end() && objectMap[id])
                {
                    P<MultiplayerObject> obj = objectMap[id];
                    if (idx < int32_t(obj->memberReplicationInfo.size()))
                        (obj->memberReplicationInfo[idx].receiveFunction)(obj->memberReplicationInfo[idx].ptr, packet);
                }
            }
            break;
        case CMD_SET_CLIENT_ID:
            packet >> clientId;
            break;
        case CMD_SET_GAME_SPEED:
            {
                float gamespeed;
                packet >> gamespeed;
                engine->setGameSpeed(gamespeed);
            }
            break;
        default:
            printf("Unknown command from server: %d\n", command);
        }
    }
}

ServerScanner::ServerScanner(int versionNumber, int serverPort)
: serverPort(serverPort), versionNumber(versionNumber)
{
    int portNr = serverPort + 1;
    while(socket.bind(portNr) != sf::UdpSocket::Done)
        portNr++;
    
    socket.setBlocking(false);
    broadcastDelay = 0.0;
}

void ServerScanner::update(float gameDelta)
{
    //Calculate our own delta, as we want wall-time delta, the gameDelta can be modified by the current game speed (could even be 0 on pause)
    float delta = updateTimeClock.getElapsedTime().asSeconds();
    updateTimeClock.restart();

    broadcastDelay -= delta;
    if (broadcastDelay < 0.0)
    {
        sf::Packet sendPacket;
        sendPacket << multiplayerMagicNumber << "ServerQuery" << int32_t(versionNumber);
        socket.send(sendPacket, sf::IpAddress::Broadcast, serverPort);
        broadcastDelay += 5.0;
    }
    
    for(unsigned int n=0; n<serverList.size(); n++)
    {
        serverList[n].timeout += delta;
        if (serverList[n].timeout > serverTimeout)
        {
            serverList.erase(serverList.begin() + n);
            n--;
        }
    }

    sf::IpAddress recvAddress;
    unsigned short recvPort;
    sf::Packet recvPacket;
    if (socket.receive(recvPacket, recvAddress, recvPort) == sf::UdpSocket::Done)
    {
        int32_t magic, versionNr;
        string name;
        recvPacket >> magic >> versionNr >> name;
        if (magic == multiplayerMagicNumber && versionNr == versionNumber)
        {
            bool found = false;
            for(unsigned int n=0; n<serverList.size(); n++)
            {
                if (serverList[n].address == recvAddress)
                {
                    serverList[n].timeout = 0.0;
                    serverList[n].name = name;
                    found = true;
                }
            }
            if (!found)
            {
                printf("ServerScanner::New server: %s %i %s\n", recvAddress.toString().c_str(), recvPort, name.c_str());
                ServerInfo si;
                si.address = recvAddress;
                si.timeout = 0.0;
                si.name = name;
                serverList.push_back(si);
            }
        }
    }
}

std::vector<ServerScanner::ServerInfo> ServerScanner::getServerList()
{
    return serverList;
}

MultiplayerObject::MultiplayerObject(string multiplayerClassIdentifier)
: multiplayerClassIdentifier(multiplayerClassIdentifier)
{
    multiplayerObjectId = noId;
    replicated = false;
    
    if (gameServer)
    {
        on_server = true;
        gameServer->registerObject(this);
    }else{
        on_server = false;
    }
}

template <> bool multiplayerReplicationFunctions<string>::isChanged(void* data, void* prev_data_ptr)
{
    string* ptr = (string*)data;
    int64_t* hash_ptr = (int64_t*)prev_data_ptr;
    
    int64_t hash = 5381;
    hash = ((hash << 5) + hash) + ptr->length();
    for(unsigned int n=0; n<ptr->length(); n++)
        hash = (hash * 33) + (*ptr)[n];
    if (*hash_ptr != hash)
    {
        *hash_ptr = hash;
        return true;
    }
    return false;
}

bool collisionable_isChanged(void* data, void* prev_data_ptr)
{
    int32_t* hashPtr = (int32_t*)prev_data_ptr;
    Collisionable* c = (Collisionable*)data;
    
    sf::Vector2f position = c->getPosition();
    sf::Vector2f velocity = c->getVelocity();
    float rotation = c->getRotation();
    float angularVelocity = c->getAngularVelocity();
    
    int32_t hash0 = position.x * 10 + ceilf(velocity.x * 100) + rotation * 100;
    int32_t hash1 = position.y * 10 + ceilf(velocity.y * 100) + angularVelocity * 100;
    if (hash0 != hashPtr[0] || hash1 != hashPtr[1])
    {
        hashPtr[0] = hash0;
        hashPtr[1] = hash1;
        return true;
    }
    return false;
}

void collisionable_sendFunction(void* data, sf::Packet& packet)
{
    Collisionable* c = (Collisionable*)data;
    
    sf::Vector2f position = c->getPosition();
    sf::Vector2f velocity = c->getVelocity();
    float rotation = c->getRotation();
    float angularVelocity = c->getAngularVelocity();
    
    packet << position.x << position.y << velocity.x << velocity.y << rotation << angularVelocity;
}

void collisionable_receiveFunction(void* data, sf::Packet& packet)
{
    Collisionable* c = (Collisionable*)data;
    
    sf::Vector2f position;
    sf::Vector2f velocity;
    float rotation;
    float angularVelocity;

    packet >> position.x >> position.y >> velocity.x >> velocity.y >> rotation >> angularVelocity;
    
    c->setPosition(position);
    c->setVelocity(velocity);
    c->setRotation(rotation);
    c->setAngularVelocity(angularVelocity);
}


void MultiplayerObject::registerCollisionableReplication()
{
    assert(!replicated);
    assert(memberReplicationInfo.size() < 0xFFFF);

    MemberReplicationInfo info;
    info.ptr = dynamic_cast<Collisionable*>(this);
    assert(info.ptr);
    info.prev_data = -1;
    info.update_delay = 0.2;
    info.update_timeout = 0.0;
    info.isChangedFunction = &collisionable_isChanged;
    info.sendFunction = &collisionable_sendFunction;
    info.receiveFunction = &collisionable_receiveFunction;
    memberReplicationInfo.push_back(info);
}

void MultiplayerObject::sendCommand(sf::Packet& packet)
{
    if (gameServer)
    {
        onReceiveCommand(0, packet);
    }else if (gameClient)
    {
        sf::Packet p;
        p << CMD_CLIENT_COMMAND << multiplayerObjectId;
        gameClient->socket.send(p);
        gameClient->socket.send(packet);
    }
}

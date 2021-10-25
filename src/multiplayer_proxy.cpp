#include "multiplayer_proxy.h"
#include "multiplayer_internal.h"
#include "engine.h"


GameServerProxy::GameServerProxy(sp::io::network::Address hostname, int hostPort, string password, int listenPort, string proxyName)
: password(password), proxyName(proxyName)
{
    LOG(INFO) << "Starting proxy server";
    mainSocket = std::make_unique<sp::io::network::TcpSocket>();
    if (!mainSocket->connect(hostname, static_cast<uint16_t>(hostPort)))
        LOG(INFO) << "Failed to connect to server";
    else
        LOG(INFO) << "Connected to server";
    mainSocket->setBlocking(false);
    listenSocket.listen(static_cast<uint16_t>(listenPort));
    listenSocket.setBlocking(false);

    newSocket = std::make_unique<sp::io::network::TcpSocket>();
    newSocket->setBlocking(false);

    boardcastServerDelay = 0.0f;
    if (proxyName != "")
    {
        if (!broadcast_listen_socket.bind(static_cast<uint16_t>(listenPort)))
        {
            LOG(ERROR) << "Failed to listen on UDP port: " << listenPort;
        }
        if (!broadcast_listen_socket.joinMulticast(666))
        {
            LOG(ERROR) << "Failed to join multicast group for local network discovery.";
        }
        broadcast_listen_socket.setBlocking(false);
    }

    no_data_timeout.start(noDataDisconnectTime);
}

GameServerProxy::GameServerProxy(string password, int listenPort, string proxyName)
: password(password), proxyName(proxyName)
{
    LOG(INFO) << "Starting listening proxy server";
    listenSocket.listen(static_cast<uint16_t>(listenPort));
    listenSocket.setBlocking(false);

    newSocket = std::make_unique<sp::io::network::TcpSocket>();
    newSocket->setBlocking(false);

    boardcastServerDelay = 0.0f;
    if (proxyName != "")
    {
        if (!broadcast_listen_socket.bind(static_cast<uint16_t>(listenPort)))
        {
            LOG(ERROR) << "Failed to listen on UDP port: " << listenPort;
        }
        broadcast_listen_socket.setBlocking(false);
    }

    no_data_timeout.start(noDataDisconnectTime);
}

GameServerProxy::~GameServerProxy()
{
}

void GameServerProxy::destroy()
{
    clientList.clear();

    broadcast_listen_socket.close();
}

void GameServerProxy::update(float delta)
{
    if (mainSocket)
    {
        sp::io::DataBuffer packet;
        while(mainSocket->receive(packet))
        {
            no_data_timeout.start(noDataDisconnectTime);
            command_t command;
            packet >> command;
            switch(command)
            {
            case CMD_REQUEST_AUTH:
                {
                    bool requirePassword;
                    packet >> serverVersion >> requirePassword;

                    sp::io::DataBuffer reply;
                    reply << CMD_CLIENT_SEND_AUTH << int32_t(serverVersion) << string(password);
                    mainSocket->send(reply);
                }
                break;
            case CMD_SET_CLIENT_ID:
                packet >> clientId;
                break;
            case CMD_ALIVE:
                {
                    sp::io::DataBuffer reply;
                    reply << CMD_ALIVE_RESP;
                    mainSocket->send(reply);
                }
                sendAll(packet);
                break;
            case CMD_CREATE:
            case CMD_DELETE:
            case CMD_UPDATE_VALUE:
            case CMD_SET_GAME_SPEED:
            case CMD_SERVER_COMMAND:
            case CMD_AUDIO_COMM_START:
            case CMD_AUDIO_COMM_DATA:
            case CMD_AUDIO_COMM_STOP:
                sendAll(packet);
                break;
            case CMD_PROXY_TO_CLIENTS:
                {
                    while(packet.available())
                    {
                        int32_t id;
                        packet >> id;
                        targetClients.insert(id);
                    }
                }
                break;
            case CMD_SET_PROXY_CLIENT_ID:
                {
                    int32_t tempId, proxied_clientId;
                    packet >> tempId >> proxied_clientId;
                    for(auto& info : clientList)
                    {
                        if (!info.validClient && info.clientId == tempId)
                        {
                            info.validClient = true;
                            info.clientId = proxied_clientId;
                            info.receiveState = CRS_Main;
                            {
                                sp::io::DataBuffer proxied_packet;
                                proxied_packet << CMD_SET_CLIENT_ID << info.clientId;
                                info.socket->send(proxied_packet);
                            }
                        }
                    }
                }
                break;
            }
        }
        if (!mainSocket->isConnected() || no_data_timeout.isExpired())
        {
            LOG(INFO) << "Disconnected proxy";
            mainSocket->close();
            engine->shutdown();
        }
    }

    if (proxyName != "")
    {
        handleBroadcastUDPSocket(delta);
    }

    if (listenSocket.accept(*newSocket))
    {
        ClientInfo info;
        info.socket = std::move(newSocket);
        newSocket = std::make_unique<sp::io::network::TcpSocket>();
        newSocket->setBlocking(false);
        {
            sp::io::DataBuffer packet;
            packet << CMD_REQUEST_AUTH << int32_t(serverVersion) << bool(password != "");
            info.socket->send(packet);
        }
        clientList.emplace_back(std::move(info));
    }

    for(unsigned int n=0; n<clientList.size(); n++)
    {
        sp::io::DataBuffer packet;
        auto& info = clientList[n];
        while(info.socket && info.socket->receive(packet))
        {
            command_t command;
            packet >> command;
            switch(info.receiveState)
            {
            case CRS_Auth:
                switch(command)
                {
                case CMD_SERVER_CONNECT_TO_PROXY:
                    if (mainSocket)
                    {
                        info.socket->close();
                        info.socket = NULL;
                    }
                    else
                    {
                        mainSocket = std::move(info.socket);
                        no_data_timeout.start(noDataDisconnectTime);
                    }
                    break;
                case CMD_CLIENT_SEND_AUTH:
                    {
                        int32_t clientVersion;
                        string clientPassword;
                        packet >> clientVersion >> clientPassword;
                        if (mainSocket && clientVersion == serverVersion && clientPassword == password)
                        {
                            sp::io::DataBuffer serverUpdate;
                            serverUpdate << CMD_NEW_PROXY_CLIENT << info.clientId;
                            mainSocket->send(serverUpdate);
                        }
                        else
                        {
                           info.socket->close();
                           info.socket = NULL;
                        }
                    }
                    break;
                case CMD_ALIVE_RESP:
                    break;
                default:
                    LOG(ERROR) << "Unknown command from client: " << command;
                    break;
                }
                break;
            case CRS_Main:
                switch(command)
                {
                case CMD_CLIENT_COMMAND:
                    packet >> info.commandObjectId;
                    info.receiveState = CRS_Command;
                    break;
                case CMD_AUDIO_COMM_START:
                case CMD_AUDIO_COMM_DATA:
                case CMD_AUDIO_COMM_STOP:
                    {
                        int32_t client_id = 0;
                        packet >> client_id;
                        if (client_id == info.clientId)
                            mainSocket->send(packet);
                    }
                    break;
                case CMD_ALIVE_RESP:
                    break;
                default:
                    LOG(ERROR) << "Unknown command from client: " << command;
                    break;
                }
                break;
            case CRS_Command:
                {
                    sp::io::DataBuffer mainPacket;
                    mainPacket << CMD_PROXY_CLIENT_COMMAND << info.commandObjectId << info.clientId;
                    mainSocket->send(mainPacket);
                    mainSocket->send(packet);
                }
                info.receiveState = CRS_Main;
                break;
            }
        }
        if (info.socket == NULL || !info.socket->isConnected())
        {
            if (info.validClient)
            {
                sp::io::DataBuffer serverUpdate;
                serverUpdate << CMD_DEL_PROXY_CLIENT << info.clientId;
                mainSocket->send(serverUpdate);
            }
            clientList.erase(clientList.begin() + n);
            n--;
        }
    }
}

void GameServerProxy::sendAll(sp::io::DataBuffer& packet)
{
    if (targetClients.empty())
    {
        for(auto& info : clientList)
        {
            if (info.validClient && info.socket)
                info.socket->send(packet);
        }
    }
    else
    {
        for(auto& info : clientList)
        {
            if (info.validClient && info.socket && targetClients.find(info.clientId) != targetClients.end())
                info.socket->send(packet);
        }
        targetClients.clear();
    }
}

void GameServerProxy::handleBroadcastUDPSocket(float delta)
{
    sp::io::network::Address recvAddress;
    int recvPort;
    sp::io::DataBuffer recvPacket;
    if (broadcast_listen_socket.receive(recvPacket, recvAddress, recvPort))
    {
        //We do not care about what we received. Reply that we live!
        sp::io::DataBuffer sendPacket;
        sendPacket << int32_t(multiplayerVerficationNumber) << int32_t(serverVersion) << proxyName;
        broadcast_listen_socket.send(sendPacket, recvAddress, recvPort);
    }
    if (boardcastServerDelay > 0.0f)
    {
        boardcastServerDelay -= delta;
    }else{
        boardcastServerDelay = 5.0f;

        sp::io::DataBuffer sendPacket;
        sendPacket << int32_t(multiplayerVerficationNumber) << int32_t(serverVersion) << proxyName;
        broadcast_listen_socket.sendMulticast(sendPacket, 666, 35667);
    }
}

#include "multiplayer_proxy.h"
#include "multiplayer_internal.h"
#include "engine.h"


GameServerProxy::GameServerProxy(sf::IpAddress hostname, int hostPort, string password, int listenPort, string proxyName)
: password(password), proxyName(proxyName)
{
    LOG(INFO) << "Starting proxy server";
    mainSocket = std::unique_ptr<TcpSocket>(new TcpSocket());
    if (mainSocket->connect(hostname, static_cast<uint16_t>(hostPort)) != sf::Socket::Status::Done)
        LOG(INFO) << "Failed to connect to server";
    else
        LOG(INFO) << "Connected to server";
    mainSocket->setBlocking(false);
    listenSocket.listen(static_cast<uint16_t>(listenPort));
    listenSocket.setBlocking(false);

    newSocket = std::unique_ptr<TcpSocket>(new TcpSocket());
    newSocket->setBlocking(false);

    boardcastServerDelay = 0.0;
    if (proxyName != "")
    {
        if (broadcast_listen_socket.bind(static_cast<uint16_t>(listenPort)) != sf::UdpSocket::Done)
        {
            LOG(ERROR) << "Failed to listen on UDP port: " << listenPort;
        }
        broadcast_listen_socket.setBlocking(false);
    }

    lastReceiveTime.restart();
}

GameServerProxy::GameServerProxy(string password, int listenPort, string proxyName)
: password(password), proxyName(proxyName)
{
    LOG(INFO) << "Starting listening proxy server";
    listenSocket.listen(static_cast<uint16_t>(listenPort));
    listenSocket.setBlocking(false);

    newSocket = std::unique_ptr<TcpSocket>(new TcpSocket());
    newSocket->setBlocking(false);

    boardcastServerDelay = 0.0;
    if (proxyName != "")
    {
        if (broadcast_listen_socket.bind(static_cast<uint16_t>(listenPort)) != sf::UdpSocket::Done)
        {
            LOG(ERROR) << "Failed to listen on UDP port: " << listenPort;
        }
        broadcast_listen_socket.setBlocking(false);
    }

    lastReceiveTime.restart();
}

GameServerProxy::~GameServerProxy()
{
}

void GameServerProxy::destroy()
{
    clientList.clear();

    broadcast_listen_socket.unbind();
}

void GameServerProxy::update(float delta)
{
    if (mainSocket)
    {
        mainSocket->update();
        sf::Packet packet;
        sf::TcpSocket::Status socketStatus;
        while((socketStatus = mainSocket->receive(packet)) == sf::TcpSocket::Done)
        {
            lastReceiveTime.restart();
            command_t command;
            packet >> command;
            switch(command)
            {
            case CMD_REQUEST_AUTH:
                {
                    bool requirePassword;
                    packet >> serverVersion >> requirePassword;

                    sf::Packet reply;
                    reply << CMD_CLIENT_SEND_AUTH << int32_t(serverVersion) << string(password);
                    mainSocket->send(reply);
                }
                break;
            case CMD_SET_CLIENT_ID:
                packet >> clientId;
                break;
            case CMD_ALIVE:
                {
                    sf::Packet reply;
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
                    int32_t id;
                    while(packet >> id)
                    {
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
                                sf::Packet proxied_packet;
                                proxied_packet << CMD_SET_CLIENT_ID << info.clientId;
                                info.socket->send(proxied_packet);
                            }
                        }
                    }
                }
                break;
            }
        }
        if (socketStatus == sf::TcpSocket::Disconnected || lastReceiveTime.getElapsedTime().asSeconds() > noDataDisconnectTime)
        {
            LOG(INFO) << "Disconnected proxy";
            mainSocket->disconnect();
            engine->shutdown();
        }
    }

    if (proxyName != "")
    {
        handleBroadcastUDPSocket(delta);
    }

    if (listenSocket.accept(*newSocket) == sf::Socket::Status::Done)
    {
        ClientInfo info;
        info.socket = std::move(newSocket);
        newSocket = std::unique_ptr<TcpSocket>(new TcpSocket());
        newSocket->setBlocking(false);
        {
            sf::Packet packet;
            packet << CMD_REQUEST_AUTH << int32_t(serverVersion) << bool(password != "");
            info.socket->send(packet);
        }
        clientList.emplace_back(std::move(info));
    }

    for(unsigned int n=0; n<clientList.size(); n++)
    {
        sf::Packet packet;
        sf::TcpSocket::Status socketStatus{ sf::TcpSocket::Error };
        auto& info = clientList[n];
        info.socket->update();
        while(info.socket && (socketStatus = info.socket->receive(packet)) == sf::TcpSocket::Done)
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
                        info.socket->disconnect();
                        info.socket = NULL;
                    }
                    else
                    {
                        mainSocket = std::move(info.socket);
                        lastReceiveTime.restart();
                    }
                    break;
                case CMD_CLIENT_SEND_AUTH:
                    {
                        int32_t clientVersion;
                        string clientPassword;
                        packet >> clientVersion >> clientPassword;
                        if (mainSocket && clientVersion == serverVersion && clientPassword == password)
                        {
                            sf::Packet serverUpdate;
                            serverUpdate << CMD_NEW_PROXY_CLIENT << info.clientId;
                            mainSocket->send(serverUpdate);
                        }
                        else
                        {
                           info.socket->disconnect();
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
                    sf::Packet mainPacket;
                    mainPacket << CMD_PROXY_CLIENT_COMMAND << info.commandObjectId << info.clientId;
                    mainSocket->send(mainPacket);
                    mainSocket->send(packet);
                }
                info.receiveState = CRS_Main;
                break;
            }
        }
        if (socketStatus == sf::TcpSocket::Disconnected || info.socket == NULL)
        {
            if (info.validClient)
            {
                sf::Packet serverUpdate;
                serverUpdate << CMD_DEL_PROXY_CLIENT << info.clientId;
                mainSocket->send(serverUpdate);
            }
            clientList.erase(clientList.begin() + n);
            n--;
        }
    }
}

void GameServerProxy::sendAll(sf::Packet& packet)
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
    sf::IpAddress recvAddress;
    unsigned short recvPort;
    sf::Packet recvPacket;
    if (broadcast_listen_socket.receive(recvPacket, recvAddress, recvPort) == sf::Socket::Status::Done)
    {
        //We do not care about what we received. Reply that we live!
        sf::Packet sendPacket;
        sendPacket << int32_t(multiplayerVerficationNumber) << int32_t(serverVersion) << proxyName;
        broadcast_listen_socket.send(sendPacket, recvAddress, recvPort);
    }
    if (boardcastServerDelay > 0.0)
    {
        boardcastServerDelay -= delta;
    }else{
        boardcastServerDelay = 5.0;

        sf::Packet sendPacket;
        sendPacket << int32_t(multiplayerVerficationNumber) << int32_t(serverVersion) << proxyName;
        UDPbroadcastPacket(broadcast_listen_socket, sendPacket, broadcast_listen_socket.getLocalPort() + 1);
    }
}

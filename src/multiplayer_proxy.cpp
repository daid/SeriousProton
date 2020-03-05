#include "multiplayer_proxy.h"
#include "multiplayer_internal.h"

GameServerProxy::GameServerProxy(sf::IpAddress hostname, int hostPort, string password, int listenPort)
: password(password)
{
    mainSocket.connect(hostname, hostPort);
    mainSocket.setBlocking(false);
    listenSocket.listen(listenPort);
    listenSocket.setBlocking(false);

    newSocket = std::unique_ptr<TcpSocket>(new TcpSocket());
    newSocket->setBlocking(false);

    lastReceiveTime.restart();
}

GameServerProxy::~GameServerProxy()
{
}

void GameServerProxy::destroy()
{
    clientList.clear();
}

void GameServerProxy::update(float delta)
{
    mainSocket.update();
    sf::Packet packet;
    sf::TcpSocket::Status socketStatus;
    while((socketStatus = mainSocket.receive(packet)) == sf::TcpSocket::Done)
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
                mainSocket.send(reply);
            }
            break;
        case CMD_SET_CLIENT_ID:
            packet >> clientId;
            break;
        case CMD_ALIVE:
            {
                sf::Packet reply;
                reply << CMD_ALIVE_RESP;
                mainSocket.send(reply);
            }
            sendAll(packet);
            break;
        case CMD_CREATE:
        case CMD_DELETE:
        case CMD_UPDATE_VALUE:
        case CMD_SET_GAME_SPEED:
        case CMD_SERVER_COMMAND:
            sendAll(packet);
            break;
        case CMD_SET_PROXY_CLIENT_ID:
            {
                int32_t tempId, clientId;
                packet >> tempId >> clientId;
                for(auto& info : clientList)
                {
                    if (!info.validClient && info.clientId == tempId)
                    {
                        info.validClient = true;
                        info.clientId = clientId;
                        info.receiveState = CRS_Main;
                        {
                            sf::Packet packet;
                            packet << CMD_SET_CLIENT_ID << info.clientId;
                            info.socket->send(packet);
                        }
                    }
                }
            }
            break;
        }
    }

    if (socketStatus == sf::TcpSocket::Disconnected || lastReceiveTime.getElapsedTime().asSeconds() > noDataDisconnectTime)
    {
        mainSocket.disconnect();
    }

    if (listenSocket.accept(*newSocket))
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
                case CMD_CLIENT_SEND_AUTH:
                    {
                        int32_t clientVersion;
                        string clientPassword;
                        packet >> clientVersion >> clientPassword;
                        if (clientVersion == serverVersion && clientPassword == password)
                        {
                            sf::Packet serverUpdate;
                            serverUpdate << CMD_NEW_PROXY_CLIENT << info.clientId;
                            mainSocket.send(serverUpdate);
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
                    mainSocket.send(mainPacket);
                    mainSocket.send(packet);
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
                mainSocket.send(serverUpdate);
            }
            clientList.erase(clientList.begin() + n);
            n--;
        }
    }
}

void GameServerProxy::sendAll(sf::Packet& packet)
{
    for(auto& info : clientList)
    {
        if (info.validClient && info.socket)
            info.socket->send(packet);
    }
}

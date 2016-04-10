#include "multiplayer_server_scanner.h"

ServerScanner::ServerScanner(int version_number, int server_port)
: server_port(server_port), version_number(version_number), master_server_scan_thread(&ServerScanner::masterServerScanThread, this)
{
}

ServerScanner::~ServerScanner()
{
    destroy();
    master_server_scan_thread.wait();
}

void ServerScanner::scanMasterServer(string url)
{
    socket.unbind();

    server_list_mutex.lock();
    for(unsigned int n=0; n<server_list.size(); n++)
    {
        if (removedServerCallback)
            removedServerCallback(server_list[n].address);
        server_list.erase(server_list.begin() + n);
        n--;
    }
    server_list_mutex.unlock();
    
    master_server_url = url;
    master_server_scan_thread.launch();
}

void ServerScanner::scanLocalNetwork()
{
    socket.unbind();
    master_server_url = "";

    server_list_mutex.lock();
    for(unsigned int n=0; n<server_list.size(); n++)
    {
        if (removedServerCallback)
            removedServerCallback(server_list[n].address);
        server_list.erase(server_list.begin() + n);
        n--;
    }
    server_list_mutex.unlock();
    
    int port_nr = server_port + 1;
    while(socket.bind(port_nr) != sf::UdpSocket::Done)
        port_nr++;
    
    socket.setBlocking(false);
    broadcast_clock.restart();
}

void ServerScanner::update(float gameDelta)
{
    server_list_mutex.lock();
    for(unsigned int n=0; n<server_list.size(); n++)
    {
        if (server_list[n].timeout_clock.getElapsedTime().asSeconds() > ServerTimeout)
        {
            if (removedServerCallback)
                removedServerCallback(server_list[n].address);
            server_list.erase(server_list.begin() + n);
            n--;
        }
    }
    server_list_mutex.unlock();

    if (socket.getLocalPort() != 0)
    {
        if (broadcast_clock.getElapsedTime().asSeconds() > BroadcastTimeout)
        {
            sf::Packet sendPacket;
            sendPacket << multiplayerVerficationNumber << "ServerQuery" << int32_t(version_number);
            UDPbroadcastPacket(socket, sendPacket, server_port);
            broadcast_clock.restart();
        }

        sf::IpAddress recv_address;
        unsigned short recv_port;
        sf::Packet recv_packet;
        while(socket.receive(recv_packet, recv_address, recv_port) == sf::UdpSocket::Done)
        {
            int32_t verification, version_nr;
            string name;
            recv_packet >> verification >> version_nr >> name;
            if (verification == multiplayerVerficationNumber && (version_nr == version_number || version_nr == 0 || version_number == 0))
            {
                updateServerEntry(recv_address, recv_port, name);
            }
        }
    }
}

void ServerScanner::updateServerEntry(sf::IpAddress address, int port, string name)
{
    sf::Lock lock(server_list_mutex);
    
    for(unsigned int n=0; n<server_list.size(); n++)
    {
        if (server_list[n].address == address)
        {
            server_list[n].port = port;
            server_list[n].name = name;
            server_list[n].timeout_clock.restart();
            return;
        }
    }

    LOG(INFO) << "ServerScanner::New server: " << address.toString() << " " << port << " " << name;
    ServerInfo si;
    si.address = address;
    si.port = port;
    si.name = name;
    si.timeout_clock.restart();
    server_list.push_back(si);
    
    if (newServerCallback)
        newServerCallback(address, name);
}

void ServerScanner::addCallbacks(std::function<void(sf::IpAddress, string)> newServerCallback, std::function<void(sf::IpAddress)> removedServerCallback)
{
    this->newServerCallback = newServerCallback;
    this->removedServerCallback = removedServerCallback;
}

std::vector<ServerScanner::ServerInfo> ServerScanner::getServerList()
{   
    std::vector<ServerScanner::ServerInfo> ret;
    server_list_mutex.lock();
    ret = server_list;
    server_list_mutex.unlock();
    return ret;
}

void ServerScanner::masterServerScanThread()
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
    
    LOG(INFO) << "Reading servers from master server";
    
    sf::Http http(hostname);
    while(!isDestroyed() && master_server_url != "")
    {
        sf::Http::Request request(uri, sf::Http::Request::Get);
        sf::Http::Response response = http.sendRequest(request, sf::seconds(10.0f));
        
        if (response.getStatus() != sf::Http::Response::Ok)
        {
            LOG(WARNING) << "Failed to query master server (" << response.getStatus() << ")";
        }
        for(string line : string(response.getBody()).split("\n"))
        {
            std::vector<string> parts = line.split(":", 3);
            if (parts.size() == 4)
            {
                sf::IpAddress address(parts[0]);
                int port = parts[1].toInt();
                int version = parts[2].toInt();
                string name = parts[3];
                
                if (version == version_number || version == 0 || version_number == 0)
                {
                    updateServerEntry(address, port, name);
                }
            }
        }
        
        for(int n=0;n<10 && !isDestroyed() && master_server_url != ""; n++)
            sf::sleep(sf::seconds(1.0f));
    }
}

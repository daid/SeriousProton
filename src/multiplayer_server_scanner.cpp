#include "multiplayer_server_scanner.h"
#include "io/http/request.h"

ServerScanner::ServerScanner(int version_number, int server_port)
: server_port(server_port), version_number(version_number)
{
}

ServerScanner::~ServerScanner()
{
    destroy();
    if (master_server_scan_thread.joinable())
        master_server_scan_thread.join();
}

void ServerScanner::scanMasterServer(string url)
{
    if (master_server_scan_thread.joinable())
        return;
    LOG(INFO) << "Switching to master server scanning";
    if (socket)
    {
        socket = nullptr;
    }

    {
        std::lock_guard<std::mutex> guard(server_list_mutex);
        for(unsigned int n=0; n<server_list.size(); n++)
        {
            if (removedServerCallback)
                removedServerCallback(server_list[n].address);
            server_list.erase(server_list.begin() + n);
            n--;
        }
    }

    master_server_url = url;
    master_server_scan_thread = std::move(std::thread(&ServerScanner::masterServerScanThread, this));
}

void ServerScanner::scanLocalNetwork()
{
    if (socket)
        return;

    LOG(INFO) << "Switching to local server scanning";
    master_server_url = "";
    if (master_server_scan_thread.joinable())
    {
        master_server_scan_thread.join();
    }

    server_list_mutex.lock();
    for(unsigned int n=0; n<server_list.size(); n++)
    {
        if (removedServerCallback)
            removedServerCallback(server_list[n].address);
        server_list.erase(server_list.begin() + n);
        n--;
    }
    server_list_mutex.unlock();

    socket = std::make_unique<sp::io::network::UdpSocket>();
    int port_nr = server_port + 1;
    while(!socket->bind(static_cast<uint16_t>(port_nr)))
        port_nr++;
    if (!socket->joinMulticast(666))
        LOG(ERROR, "Failed to join multicast for local network discovery");

    socket->setBlocking(false);
    broadcast_timer.repeat(BroadcastTimeout);
}

void ServerScanner::update(float /*gameDelta*/)
{
    server_list_mutex.lock();
    for(unsigned int n=0; n<server_list.size(); n++)
    {
        if (server_list[n].timeout.isExpired())
        {
            if (removedServerCallback)
                removedServerCallback(server_list[n].address);
            server_list.erase(server_list.begin() + n);
            n--;
        }
    }
    server_list_mutex.unlock();

    if (socket)
    {
        if (broadcast_timer.isExpired())
        {
            sp::io::DataBuffer sendPacket;
            sendPacket << multiplayerVerficationNumber << "ServerQuery" << int32_t(version_number);
            socket->sendMulticast(sendPacket, 666, server_port);
        }

        sp::io::network::Address recv_address;
        int recv_port;
        sp::io::DataBuffer recv_packet;
        while(socket->receive(recv_packet, recv_address, recv_port))
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

void ServerScanner::updateServerEntry(sp::io::network::Address address, int port, string name)
{
    std::lock_guard<std::mutex> guard(server_list_mutex);

    for(unsigned int n=0; n<server_list.size(); n++)
    {
        if (server_list[n].address == address)
        {
            server_list[n].port = port;
            server_list[n].name = name;
            server_list[n].timeout.start(ServerTimeout);
            return;
        }
    }

    LOG(INFO) << "ServerScanner::New server: " << address.getHumanReadable()[0] << " " << port << " " << name;
    ServerInfo si;
    si.address = address;
    si.port = port;
    si.name = name;
    si.timeout.start(ServerTimeout);
    server_list.push_back(si);
    
    if (newServerCallback)
        newServerCallback(address, name);
}

void ServerScanner::addCallbacks(std::function<void(sp::io::network::Address, string)> newServerCallbackIn, std::function<void(sp::io::network::Address)> removedServerCallbackIn)
{
    this->newServerCallback = newServerCallbackIn;
    this->removedServerCallback = removedServerCallbackIn;
}

std::vector<ServerScanner::ServerInfo> ServerScanner::getServerList()
{   
    std::vector<ServerScanner::ServerInfo> ret;
    {
        std::lock_guard<std::mutex> guard(server_list_mutex);
        ret = server_list;
    }
    return ret;
}

void ServerScanner::masterServerScanThread()
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
        LOG(INFO) << "Port detected.";
        // If a port is attached to the hostname, parse it out.
        // No validation is performed.
        port = hostname.substr(port_start + 1, path_start).toInt();
        hostname = hostname.substr(0, port_start);
    }else{
        hostname = hostname.substr(0, path_start);
    }

    LOG(INFO) << "Reading servers from master server " << master_server_url;

    sp::io::http::Request http(hostname, port);
    while(!isDestroyed() && master_server_url != "")
    {
        auto response = http.get(uri);
        if (response.status != 200)
        {
            LOG(WARNING) << "Failed to query master server " << master_server_url << " (status " << response.status << ")";
        }
        for(string line : response.body.split("\n"))
        {
            std::vector<string> parts = line.split(":", 3);
            if (parts.size() == 4)
            {
                sp::io::network::Address address(parts[0]);
                int part_port = parts[1].toInt();
                int version = parts[2].toInt();
                string name = parts[3];
                
                if (version == version_number || version == 0 || version_number == 0)
                {
                    updateServerEntry(address, part_port, name);
                }
            }
        }
        
        for(int n=0;n<10 && !isDestroyed() && master_server_url != ""; n++)
            std::this_thread::sleep_for(std::chrono::duration<float>(1.f));
    }
}

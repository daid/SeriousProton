#include "multiplayer_server_scanner.h"
#include "io/http/request.h"
using namespace std::chrono_literals;

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
    abort_wait.notify_all();
    if (master_server_scan_thread.joinable())
        return;
    LOG(INFO, "Starting master server scanning");

    master_server_url = url;
    master_server_scan_thread = std::move(std::thread(&ServerScanner::masterServerScanThread, this));
}

void ServerScanner::scanLocalNetwork()
{
    if (socket)
        return;

    LOG(INFO, "Starting local server scanning");

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
    master_server_list_mutex.lock();
    for(const auto& info : master_server_update_list) {
        updateServerEntry(info);
    }
    master_server_update_list.clear();
    master_server_list_mutex.unlock();
    for(unsigned int n=0; n<server_list.size(); n++)
    {
        if (server_list[n].timeout.isExpired())
        {
            if (removedServerCallback)
                removedServerCallback(server_list[n]);
            server_list.erase(server_list.begin() + n);
            n--;
        }
    }

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
                updateServerEntry({ServerType::LAN, recv_address, uint64_t(recv_port), name, {}});
            }
        }
    }
}

void ServerScanner::updateServerEntry(const ServerInfo& info)
{
    for(unsigned int n=0; n<server_list.size(); n++)
    {
        if (server_list[n].type == info.type && server_list[n].address == info.address)
        {
            server_list[n].port = info.port;
            server_list[n].name = info.name;
            server_list[n].timeout.start(ServerTimeout);
            return;
        }
    }

    LOG(INFO) << "ServerScanner::New server: " << info.address.getHumanReadable()[0] << " " << info.port << " " << info.name;
    ServerInfo si = info;
    si.timeout.start(ServerTimeout);
    server_list.push_back(si);
    
    if (newServerCallback)
        newServerCallback(si);
}

void ServerScanner::addCallbacks(std::function<void(const ServerInfo&)> newServerCallbackIn, std::function<void(const ServerInfo&)> removedServerCallbackIn)
{
    this->newServerCallback = newServerCallbackIn;
    this->removedServerCallback = removedServerCallbackIn;
}

std::vector<ServerScanner::ServerInfo> ServerScanner::getServerList()
{   
    return server_list;
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
    string uri = hostname.substr(path_start);
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
                    master_server_list_mutex.lock();
                    master_server_update_list.push_back({ServerType::MasterServer, address, uint64_t(part_port), name, {}});
                    master_server_list_mutex.unlock();
                }
            }
        }

        if (!isDestroyed() && master_server_url != "") {
            std::mutex wait_mutex;
            std::unique_lock<std::mutex> lk(wait_mutex);
            abort_wait.wait_for(lk, 10s);
        }
    }
}

void ServerScanner::destroy() {
    PObject::destroy();
    abort_wait.notify_all();
}

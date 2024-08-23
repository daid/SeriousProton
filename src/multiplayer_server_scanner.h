#ifndef MULTIPLAYER_SERVER_SCANER_H
#define MULTIPLAYER_SERVER_SCANER_H

#include <functional>
#include "multiplayer_server.h"
#include <thread>
#include <mutex>
#include <condition_variable>


//Class to find all servers that have the correct version number. Creates a big nice list.
class ServerScanner : public Updatable
{
public:
    enum class ServerType {
        Manual,
        LAN,
        MasterServer,
        SteamFriend,
    };
    struct ServerInfo
    {
        ServerType type;
        sp::io::network::Address address;
        uint64_t port;
        string name;

        sp::SystemTimer timeout;
    };

    ServerScanner(int version_number, int server_port = defaultServerPort);
    virtual ~ServerScanner();

    virtual void destroy() override;

    virtual void update(float delta) override;
    void addCallbacks(std::function<void(const ServerInfo&)> newServerCallback, std::function<void(const ServerInfo&)> removedServerCallback);
    
    void scanLocalNetwork();
    void scanMasterServer(string url);

    std::vector<ServerInfo> getServerList();

private:
    void masterServerScanThread();
    
    void updateServerEntry(const ServerInfo& info);

    int server_port;
    std::unique_ptr<sp::io::network::UdpSocket> socket;
    sp::SystemTimer broadcast_timer;

    std::vector<ServerInfo> server_list;
    int version_number;
    constexpr static float BroadcastTimeout = 2.0f;
    constexpr static float ServerTimeout = 30.0f;

    string master_server_url;
    std::mutex master_server_list_mutex;
    std::vector<ServerInfo> master_server_update_list;
    std::thread master_server_scan_thread;
    std::condition_variable abort_wait;

    std::function<void(const ServerInfo&)> newServerCallback;
    std::function<void(const ServerInfo&)> removedServerCallback;
};

#endif//MULTIPLAYER_SERVER_SCANER_H

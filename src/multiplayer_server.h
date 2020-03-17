#ifndef MULTIPLAYER_SERVER_H
#define MULTIPLAYER_SERVER_H

#include <stdint.h>
#include <unordered_map>
#include <unordered_set>
#include "fixedSocket.h"
#include "Updatable.h"
#include "stringImproved.h"
#include "networkAudioStream.h"

static const int defaultServerPort = 35666;
static const int multiplayerVerficationNumber = 0x2fab3f0f; //Used to verify that the server is actually a serious proton server

class GameServer;
class MultiplayerObject;

extern P<GameServer> game_server;

class GameServer : public Updatable
{
    sf::Clock updateTimeClock;
    sf::Clock aliveClock;
    sf::UdpSocket broadcast_listen_socket;
    sf::TcpListener listenSocket;
    std::unique_ptr<TcpSocket> new_socket;
    string server_name;
    int listen_port;
    int version_number;
    string server_password;
    
    int sendDataCounter;
    int sendDataCounterPerClient;
    float sendDataRate;
    float sendDataRatePerClient;
    float update_run_time;
    
    float lastGameSpeed;
    float boardcastServerDelay;

    enum EClientReceiveState
    {
        CRS_Auth,
        CRS_Main,
        CRS_Command
    };
    struct ClientInfo
    {
        std::unique_ptr<TcpSocket> socket;
        int32_t client_id;
        int32_t command_client_id;
        EClientReceiveState receive_state;
        int32_t command_object_id;
        sf::Clock round_trip_time;
        int32_t ping;
        std::vector<int32_t> proxy_ids;
    };
    int32_t nextclient_id;
    std::vector<ClientInfo> clientList;
    std::unordered_map<int32_t, std::unordered_set<int32_t>> voice_targets;
    NetworkAudioStreamManager audio_stream_manager;

    int32_t nextObjectId;
    std::unordered_map<int32_t, P<MultiplayerObject> > objectMap;

    string master_server_url;
    sf::Thread master_server_update_thread;
public:
    GameServer(string server_name, int versionNumber, int listenPort = defaultServerPort);
    virtual ~GameServer();

    virtual void destroy() override;

    P<MultiplayerObject> getObjectById(int32_t id);
    virtual void update(float delta);
    inline float getSendDataRate() { return sendDataRate; }
    inline float getSendDataRatePerClient() { return sendDataRatePerClient; }
    inline float getUpdateTime() { return update_run_time; }

    string getServerName() { return server_name; }
    void setServerName(string name) { server_name = name; }
    
    void registerOnMasterServer(string master_server_url);
    void stopMasterServerRegistry();
    void setPassword(string password);

    void startAudio(int32_t client_id, int32_t target_identifier);
    void gotAudioPacket(int32_t client_id, const unsigned char* packet, int packet_size);
    void stopAudio(int32_t client_id);
    void sendAudioPacketFrom(int32_t client_id, sf::Packet& packet);
private:
    void registerObject(P<MultiplayerObject> obj);
    void broadcastServerCommandFromObject(int32_t id, sf::Packet& packet);
    void keepAliveAll();
    void sendAll(sf::Packet& packet);

    void generateCreatePacketFor(P<MultiplayerObject> obj, sf::Packet& packet);
    void generateDeletePacketFor(int32_t id, sf::Packet& packet);
    
    void handleNewClient(ClientInfo& info);
    void handleNewProxy(ClientInfo& info, int32_t temp_id);
    
    void runMasterServerUpdateThread();
    
    void handleBroadcastUDPSocket(float delta);

    friend class MultiplayerObject;
public:
    virtual void onNewClient(int32_t client_id) {}
    virtual void onDisconnectClient(int32_t client_id) {}
    virtual std::unordered_set<int32_t> onVoiceChat(int32_t client_id, int32_t target_identifier);
};

#endif//MULTIPLAYER_SERVER_H

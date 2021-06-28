#ifndef MULTIPLAYER_CLIENT_H
#define MULTIPLAYER_CLIENT_H

#include "io/network/tcpSocket.h"
#include "Updatable.h"
#include "multiplayer_server.h"
#include "networkAudioStream.h"
#include "timer.h"

#include <stdint.h>
#include <thread>


class GameClient;
class MultiplayerObject;

extern P<GameClient> game_client;

class GameClient : public Updatable
{
    constexpr static float no_data_disconnect_time = 20;
public:
    enum Status
    {
        ReadyToConnect,
        Connecting,
        Authenticating,
        WaitingForPassword,
        Connected,
        Disconnected
    };

    enum class DisconnectReason : uint8_t
    {
        None = 0,
        VersionMismatch,
        BadCredentials,
        TimedOut,
        ClosedByServer, // Normal termination.
        Unknown
    };
private:
    int version_number;
    sp::io::network::Address server;
    int port_nr;

    sp::io::network::TcpSocket socket;
    std::unordered_map<int32_t, P<MultiplayerObject> > objectMap;
    int32_t client_id;
    Status status;
    sp::SystemTimer no_data_timeout;
    NetworkAudioStreamManager audio_stream_manager;

    std::thread connect_thread;
    DisconnectReason disconnect_reason{ DisconnectReason::Unknown };
public:
    GameClient(int version_number, sp::io::network::Address server, int port_nr = defaultServerPort);
    virtual ~GameClient();

    P<MultiplayerObject> getObjectById(int32_t id);
    virtual void update(float delta);

    int32_t getClientId() { return client_id; }
    Status getStatus() { return status; }
    DisconnectReason getDisconnectReason() const { return disconnect_reason; }

    void sendPacket(sp::io::DataBuffer& packet);

    void sendPassword(string password);
private:
    void runConnect();
};

#endif//MULTIPLAYER_CLIENT_H

#ifndef MULTIPLAYER_CLIENT_H
#define MULTIPLAYER_CLIENT_H

#include "io/network/streamSocket.h"
#include "Updatable.h"
#include "multiplayer_server.h"
#include "networkAudioStream.h"
#include "timer.h"
#include "ecs/entity.h"

#include <stdint.h>
#include <thread>


class GameClient;
class MultiplayerObject;

extern P<GameClient> game_client;

class GameClient : public Updatable
{
    // if the server doesn't send us any data for this long, send a packet to see if it's still there
    constexpr static float heartbeat_time = 0.5;
    // if the server doesn't send us any data for this long, disconnect
    constexpr static float no_data_disconnect_time = 20;
public:
    std::vector<sp::ecs::Entity> entity_mapping;

    enum Status
    {
        Connecting,
        Authenticating,
        WaitingForPassword,
        Connected,
        Disconnected
    };

    enum class DisconnectReason : uint8_t
    {
        None = 0,
        FailedToConnect,
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

    std::unique_ptr<sp::io::network::StreamSocket> socket;
    std::unordered_map<int32_t, P<MultiplayerObject> > objectMap;
    int32_t client_id;
    Status status;
    sp::SystemTimer no_data_timeout;
    sp::SystemTimer heartbeat_timer;
    NetworkAudioStreamManager audio_stream_manager;

    DisconnectReason disconnect_reason{ DisconnectReason::Unknown };
public:
    GameClient(int version_number, sp::io::network::Address server, int port_nr = defaultServerPort);
#ifdef STEAMSDK
    GameClient(int version_number, uint64_t steam_id);
#endif
    virtual ~GameClient();

    P<MultiplayerObject> getObjectById(int32_t id);
    virtual void update(float delta) override;

    int32_t getClientId() { return client_id; }
    Status getStatus() { return status; }
    DisconnectReason getDisconnectReason() const { return disconnect_reason; }

    void sendPacket(sp::io::DataBuffer& packet);

    void sendPassword(string password);
};

#endif//MULTIPLAYER_CLIENT_H

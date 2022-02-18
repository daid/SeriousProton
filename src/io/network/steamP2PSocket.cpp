#include <io/network/steamP2PSocket.h>
#include <logging.h>
#include <steam/steam_api.h>

namespace sp {
namespace io {
namespace network {

SteamP2PSocket::SteamP2PSocket()
{
}

SteamP2PSocket::~SteamP2PSocket()
{
    close();
}

bool SteamP2PSocket::connect(uint64_t steam_id)
{
    close();
    SteamNetworkingIdentity id;
    id.SetSteamID64(steam_id);
    handle = SteamNetworkingSockets()->ConnectP2P(id, 0, 0, nullptr);
    return handle != 0;
}

void SteamP2PSocket::close()
{
    if (handle)
    {
        SteamNetworkingSockets()->CloseConnection(handle, 0, nullptr, false);
        handle = 0;
    }
}

StreamSocket::State SteamP2PSocket::getState()
{
    if (handle == 0)
        return StreamSocket::State::Closed;
    SteamNetConnectionRealTimeStatus_t status;
    if (SteamNetworkingSockets()->GetConnectionRealTimeStatus(handle, &status, 0, nullptr) != k_EResultOK)
    {
        close();
        return StreamSocket::State::Closed;
    }
    switch(status.m_eState)
    {
	case k_ESteamNetworkingConnectionState_Connecting:
    case k_ESteamNetworkingConnectionState_FindingRoute:
        return StreamSocket::State::Connecting;
	case k_ESteamNetworkingConnectionState_Connected:
        return StreamSocket::State::Connected;
    case k_ESteamNetworkingConnectionState_None:
    case k_ESteamNetworkingConnectionState_ClosedByPeer:
    case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
    default:
        close();
        return StreamSocket::State::Closed;
    }
    return StreamSocket::State::Connected;
}

size_t SteamP2PSocket::_send(const void* data, size_t size)
{
    if (!handle)
        return 0;
    if (SteamNetworkingSockets()->SendMessageToConnection(handle, data, size, 0, nullptr) == k_EResultOK)
        return size;
    return 0;
}

size_t SteamP2PSocket::_receive(void* data, size_t size)
{
    if (!handle)
        return 0;
    if (!recv_buffer.empty())
    {
        if (size > recv_buffer.size())
            size = recv_buffer.size();
        memcpy(data, recv_buffer.data(), size);
        memmove(recv_buffer.data(), recv_buffer.data() + size, recv_buffer.size() - size);
        recv_buffer.resize(recv_buffer.size() - size);
        return size;
    }
    SteamNetworkingMessage_t* message;
    auto result = SteamNetworkingSockets()->ReceiveMessagesOnConnection(handle, &message, 1);
    if (result < 0)
    {
        close();
        return 0;
    }
    if (result == 0)
        return 0;

    if (size > message->GetSize())
        size = message->GetSize();
    memcpy(data, message->GetData(), size);
    if (size < message->GetSize())
    {
        recv_buffer.resize(message->GetSize() - size);
        memcpy(recv_buffer.data(), reinterpret_cast<const uint8_t*>(message->GetData()) + size, message->GetSize() - size);
    }
    message->Release();
    return size;
}

}//namespace network
}//namespace io
}//namespace sp

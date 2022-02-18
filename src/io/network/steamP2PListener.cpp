#include <io/network/steamP2PListener.h>
#include <io/network/steamP2PSocket.h>
#include <steam/steam_api.h>
#include <vector>


namespace sp {
namespace io {
namespace network {

static std::vector<uint32_t> new_connections;

static void OnSteamNetConnectionStatusChanged( SteamNetConnectionStatusChangedCallback_t *pInfo )
{
    switch(pInfo->m_info.m_eState)
	{
    case k_ESteamNetworkingConnectionState_Connecting:
        if (pInfo->m_info.m_hListenSocket) {
            new_connections.push_back(pInfo->m_hConn);
			SteamNetworkingSockets()->AcceptConnection( pInfo->m_hConn );
        }
        break;
    default:
        break;
    }
}

SteamP2PListener::SteamP2PListener()
{
	SteamNetworkingUtils()->SetGlobalCallback_SteamNetConnectionStatusChanged(OnSteamNetConnectionStatusChanged);
}

SteamP2PListener::~SteamP2PListener()
{
    close();
}

bool SteamP2PListener::listen()
{
    if (handle)
        return true;
    handle = SteamNetworkingSockets()->CreateListenSocketP2P(0, 0, nullptr);
    return handle != 0;
}

void SteamP2PListener::close()
{
    if (isListening())
    {
        SteamNetworkingSockets()->CloseListenSocket(handle);
        handle = 0;
    }
}

bool SteamP2PListener::isListening()
{
    return handle != 0;
}

std::unique_ptr<SteamP2PSocket> SteamP2PListener::accept()
{
    if (new_connections.empty())
        return nullptr;
    auto result = std::make_unique<SteamP2PSocket>();
    result->handle = new_connections.back();
    new_connections.pop_back();
    return result;
}

}//namespace network
}//namespace io
}//namespace sp

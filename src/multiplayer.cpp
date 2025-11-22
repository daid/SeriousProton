#include "multiplayer.h"
#include "multiplayer_client.h"
#include "engine.h"
#include "multiplayer_internal.h"
#include "components/multiplayer.h"


namespace sp::io {
    DataBuffer& operator << (DataBuffer& packet, const sp::ecs::Entity& e)
    {
        if (game_client) {
            auto si = e.getComponent<ServerIndex>();
            if (!si)
                packet << sp::ecs::Entity::no_index << sp::ecs::Entity::no_index;
            else
                packet << si->index << si->version;
        } else {
            packet << e.getIndex() << e.getVersion();
        }
        return packet;
    }

    DataBuffer& operator >> (DataBuffer& packet, sp::ecs::Entity& e)
    {
        uint32_t index, version;
        packet >> index >> version;
        if (game_client) {
            if (index < game_client->entity_mapping.size())
                e = game_client->entity_mapping[index];
            else
                e = {};
        } else {
            e = sp::ecs::Entity::forced(index, version);
        }
        return packet;
    }
}

MultiplayerClassListItem* multiplayerClassListStart;

MultiplayerObject::MultiplayerObject(string multiplayerClassIdentifier)
: multiplayerClassIdentifier(multiplayerClassIdentifier)
{
    multiplayerObjectId = noId;
    replicated = false;

    if (game_server)
    {
        on_server = true;
        game_server->registerObject(this);
    }else{
        on_server = false;
    }
}

MultiplayerObject::~MultiplayerObject()
{
    for(unsigned int n=0; n<memberReplicationInfo.size(); n++)
        if (memberReplicationInfo[n].cleanupFunction)
            memberReplicationInfo[n].cleanupFunction(&memberReplicationInfo[n].prev_data);
}

template <> bool multiplayerReplicationFunctions<string>::isChanged(void* data, void* prev_data_ptr)
{
    string* ptr = (string*)data;
    uintptr_t* hash_ptr = (uintptr_t*)prev_data_ptr;

    uintptr_t hash = 5381;
    hash = ((hash << 5) + hash) + ptr->length();
    for(unsigned int n=0; n<ptr->length(); n++)
        hash = (hash * 33) + (*ptr)[n];
    if (*hash_ptr != hash)
    {
        *hash_ptr = hash;
        return true;
    }
    return false;
}

template <> void multiplayerReplicationFunctions<sp::ecs::Entity>::sendData(void* data, sp::io::DataBuffer& packet)
{
    auto e = (sp::ecs::Entity*)data;
    if (*e) {
        packet << e->getIndex();
    } else {
        packet << sp::ecs::Entity::no_index;
    }
}

template <> void multiplayerReplicationFunctions<sp::ecs::Entity>::receiveData(void* data, sp::io::DataBuffer& packet)
{
    auto e = (sp::ecs::Entity*)data;

    uint32_t index;
    packet >> index;
    if (index < game_client->entity_mapping.size())
        *e = game_client->entity_mapping[index];
    else
        *e = sp::ecs::Entity{};
}

// Explicit template instantiation to ensure symbols are exported
template void multiplayerReplicationFunctions<sp::ecs::Entity>::sendData(void*, sp::io::DataBuffer&);
template void multiplayerReplicationFunctions<sp::ecs::Entity>::receiveData(void*, sp::io::DataBuffer&);

void MultiplayerObject::sendClientCommand(sp::io::DataBuffer& packet)
{
    if (game_server)
    {
        onReceiveClientCommand(0, packet);
    }else if (game_client)
    {
        sp::io::DataBuffer p;
        p << CMD_CLIENT_COMMAND << multiplayerObjectId;
        game_client->sendPacket(p);
        game_client->sendPacket(packet);
    }
}

void MultiplayerObject::broadcastServerCommand(sp::io::DataBuffer& packet)
{
    if (game_server)
    {
        onReceiveServerCommand(packet);
        game_server->broadcastServerCommandFromObject(multiplayerObjectId, packet);
    }
}

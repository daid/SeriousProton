#include "multiplayer.h"
#include "multiplayer_client.h"
#include "collisionable.h"
#include "engine.h"
#include "multiplayer_internal.h"

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
    int64_t* hash_ptr = (int64_t*)prev_data_ptr;

    int64_t hash = 5381;
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

bool collisionable_isChanged(void* data, void* prev_data_ptr)
{
    int32_t* hashPtr = (int32_t*)prev_data_ptr;
    Collisionable* c = (Collisionable*)data;

    sf::Vector2f position = c->getPosition();
    sf::Vector2f velocity = c->getVelocity();
    float rotation = c->getRotation();
    float angularVelocity = c->getAngularVelocity();

    int32_t hash0 = position.x * 10 + ceilf(velocity.x * 100) + rotation * 100;
    int32_t hash1 = position.y * 10 + ceilf(velocity.y * 100) + angularVelocity * 100;
    if (hash0 != hashPtr[0] || hash1 != hashPtr[1])
    {
        hashPtr[0] = hash0;
        hashPtr[1] = hash1;
        return true;
    }
    return false;
}

void collisionable_sendFunction(void* data, sf::Packet& packet)
{
    Collisionable* c = (Collisionable*)data;

    sf::Vector2f position = c->getPosition();
    sf::Vector2f velocity = c->getVelocity();
    float rotation = c->getRotation();
    float angularVelocity = c->getAngularVelocity();

    packet << position << velocity << rotation << angularVelocity;
}

void collisionable_receiveFunction(void* data, sf::Packet& packet)
{
    Collisionable* c = (Collisionable*)data;

    sf::Vector2f position;
    sf::Vector2f velocity;
    float rotation;
    float angularVelocity;

    packet >> position >> velocity >> rotation >> angularVelocity;

    c->setPosition(position);
    c->setVelocity(velocity);
    c->setRotation(rotation);
    c->setAngularVelocity(angularVelocity);
}


void MultiplayerObject::registerCollisionableReplication()
{
    assert(!replicated);
    assert(memberReplicationInfo.size() < 0xFFFF);

    MemberReplicationInfo info;
    info.ptr = dynamic_cast<Collisionable*>(this);
    assert(info.ptr);
#ifdef DEBUG
    info.name = "Collisionable_data";
#endif
    info.prev_data = -1;
    info.update_delay = 0.4;
    info.update_timeout = 0.0;
    info.isChangedFunction = &collisionable_isChanged;
    info.sendFunction = &collisionable_sendFunction;
    info.receiveFunction = &collisionable_receiveFunction;
    info.cleanupFunction = NULL;
    memberReplicationInfo.push_back(info);
}

void MultiplayerObject::sendClientCommand(sf::Packet& packet)
{
    if (game_server)
    {
        onReceiveClientCommand(0, packet);
    }else if (game_client)
    {
        sf::Packet p;
        p << CMD_CLIENT_COMMAND << multiplayerObjectId;
        while(game_client->socket.send(p) == sf::TcpSocket::NotReady) {}
        while(game_client->socket.send(packet) == sf::TcpSocket::NotReady) {}
    }
}

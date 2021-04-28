#include "multiplayer.h"
#include "multiplayer_client.h"
#include "collisionable.h"
#include "engine.h"
#include "multiplayer_internal.h"

static PVector<Collisionable> collisionable_significant;
class CollisionableReplicationData
{
public:
    sf::Vector2f position;
    sf::Vector2f velocity;
    float rotation;
    float angularVelocity;
    sf::Clock last_update_time;
    
    CollisionableReplicationData()
    : rotation(0), angularVelocity(0)
    {
        last_update_time.restart();
    }
};


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

static bool collisionable_isChanged(void* data, void* prev_data_ptr)
{
    CollisionableReplicationData* rep_data = *(CollisionableReplicationData**)prev_data_ptr;
    Collisionable* c = (Collisionable*)data;

    sf::Vector2f position = c->getPosition();
    sf::Vector2f velocity = c->getVelocity();
    float rotation = c->getRotation();
    float angular_velocity = c->getAngularVelocity();
    float time_after_update = rep_data->last_update_time.getElapsedTime().asSeconds();
    float significance = 0.f;
    float significant_range = 1.f;

    foreach(Collisionable, sig, collisionable_significant)
    {
        float dist = sf::length(sig->getPosition() - position);
        float s = 0.f;
        if (dist < sig->multiplayer_replication_object_significant_range)
            s = 1.f;
        else if (dist < sig->multiplayer_replication_object_significant_range * 2.f)
            s = 1.f - ((dist - sig->multiplayer_replication_object_significant_range) / sig->multiplayer_replication_object_significant_range);
        
        if (s > significance)
        {
            significance = s;
            significant_range = sig->multiplayer_replication_object_significant_range;
        }
    }
    
    float delta_position = sf::length(rep_data->position - position);
    float delta_velocity = sf::length(rep_data->velocity - velocity);
    float delta_rotation = fabs(rep_data->rotation - rotation);
    float delta_angular_velocity = fabs(rep_data->angularVelocity - angular_velocity);
    
    if (delta_position == 0.f && delta_velocity == 0.f && delta_rotation == 0.f && delta_angular_velocity == 0.f)
    {
        //If we haven't moved then the client needs no update
        rep_data->last_update_time.restart();
    }else{
        float position_score = (delta_position / significant_range) * 100.f + delta_rotation * 10.f;
        float velocity_score = (delta_velocity / significant_range) * 100.f + delta_angular_velocity * 10.f;
        
        float time_between_updates = (1.f - (position_score + velocity_score) * significance);
        if (time_between_updates < 0.05f)
            time_between_updates = 0.05f;
        if (time_after_update > 0.5f || time_after_update > time_between_updates)
        {
            rep_data->last_update_time.restart();
            rep_data->position = position;
            rep_data->velocity = velocity;
            rep_data->rotation = rotation;
            rep_data->angularVelocity = angular_velocity;
            return true;
        }
    }
    return false;
}

static void collisionable_sendFunction(void* data, sf::Packet& packet)
{
    Collisionable* c = (Collisionable*)data;

    sf::Vector2f position = c->getPosition();
    sf::Vector2f velocity = c->getVelocity();
    float rotation = c->getRotation();
    float angularVelocity = c->getAngularVelocity();

    packet << position << velocity << rotation << angularVelocity;
}

static void collisionable_receiveFunction(void* data, sf::Packet& packet)
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

static void collisionable_cleanupFunction(void* prev_data_ptr)
{
    CollisionableReplicationData* rep_data = *(CollisionableReplicationData**)prev_data_ptr;
    delete rep_data;
}

void MultiplayerObject::registerCollisionableReplication(float object_significant_range)
{
    assert(!replicated);
    assert(memberReplicationInfo.size() < 0xFFFF);

    MemberReplicationInfo info;
    Collisionable* collisionable = dynamic_cast<Collisionable*>(this);
    assert(collisionable);
    collisionable->multiplayer_replication_object_significant_range = object_significant_range;
    if (object_significant_range > 0)
        collisionable_significant.push_back(collisionable);
    info.ptr = collisionable;
#ifdef DEBUG
    info.name = "Collisionable_data";
#endif
    info.prev_data = reinterpret_cast<std::uint64_t>(new CollisionableReplicationData());
    info.update_delay = 0.f;
    info.update_timeout = 0.f;
    info.isChangedFunction = &collisionable_isChanged;
    info.sendFunction = &collisionable_sendFunction;
    info.receiveFunction = &collisionable_receiveFunction;
    info.cleanupFunction = &collisionable_cleanupFunction;
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
        game_client->sendPacket(p);
        game_client->sendPacket(packet);
    }
}

void MultiplayerObject::broadcastServerCommand(sf::Packet& packet)
{
    if (game_server)
    {
        onReceiveServerCommand(packet);
        game_server->broadcastServerCommandFromObject(multiplayerObjectId, packet);
    }
}

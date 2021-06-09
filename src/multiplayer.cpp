#include "multiplayer.h"
#include "multiplayer_client.h"
#include "collisionable.h"
#include "engine.h"
#include "multiplayer_internal.h"

namespace replication
{
    Item::Item(ControlBlock& control, const Settings& settings)
    : controller{ control }
    {
        controller.add(*this, settings);
    }

    Item::~Item() = default;

    void Item::setId(Key, uint16_t id)
    {
        replication_id = id;
    }

    ControlBlock& Item::getController(Key) const
    {
        return controller;
    }

    uint16_t Item::getId(Key) const
    {
        return replication_id;
    }

    void Item::setDirty()
    {
        controller.setDirty(*this);
    }

    void ControlBlock::add(Item& item, const Item::Settings& settings)
    {
        assert(items.size() < static_cast<size_t>(~controlled_flag));
        item.setId({}, static_cast<uint16_t>(items.size()));
        items.emplace_back(&item);
        min_update_interval.emplace_back(settings.min_delay);
        time_since_last_update.emplace_back(0.f);
        if (dirty.size() * sizeof(size_t) < items.size())
            dirty.resize((items.size() + sizeof(size_t) - 1) / sizeof(size_t));

#ifdef DEBUG
            names.emplace_back(settings.name);
#endif
    }

    bool ControlBlock::isDirty(const Item& item) const
    {
        assert(&item.getController({}) == this);
        return isDirty(item.getId({}));
    }

    void ControlBlock::setDirty(const Item& item)
    {
        assert(&item.getController({}) == this);
        setDirty(item.getId({}));
    }

    size_t ControlBlock::send(sf::Packet& packet, bool everything, float delta /* = 0.f */)
    {
        size_t sent{};

        // Update all deltas.
        for (auto& value : time_since_last_update)
            value += delta;

        if (!everything)
        {
            for (auto dirty_batch = 0; dirty_batch < dirty.size(); ++dirty_batch)
            {
                auto batch = dirty[dirty_batch];
                if (batch) // skip over entire batches with no change.
                {
                    for (auto i = 0; i < sizeof(size_t); ++i)
                    {
                        auto bit = (size_t{ 1 } << i);
                        auto index = dirty_batch * sizeof(size_t) + i;
                        if ((batch & bit) != 0 && time_since_last_update[index] > min_update_interval[index])
                        {
                            auto item = items[index];
                            packet << int16_t(item->getId({}) | controlled_flag);
                            item->send(*this, packet);
                            time_since_last_update[index] = 0.f;
                            sent += 1;
                            batch &= ~bit;
                        }
                    }

                    if (batch != dirty[dirty_batch])
                    {
                        dirty[dirty_batch] = batch;
                    }
                }
            }
        }
        else
        {
            for (auto& item : items)
            {
                packet << int16_t(item->getId({}) | controlled_flag);
                item->send(*this, packet);
            }
            sent = items.size();
        }

        return sent;
    }

    bool ControlBlock::handles(uint16_t net_id) const
    {
        return (net_id & controlled_flag) && (net_id & ~controlled_flag) < items.size();
    }

    void ControlBlock::receive(uint16_t net_id, sf::Packet& packet)
    {
        auto element_key = net_id & ~controlled_flag;
        items[element_key]->receive(*this, packet);
    }


    bool ControlBlock::isDirty(size_t id) const
    {
        return (dirty[id / sizeof(size_t)] & (size_t{ 1 } << (id % sizeof(size_t)))) != 0;
    }

    void ControlBlock::setDirty(size_t id)
    {
        dirty[id / sizeof(size_t)] |= (size_t{ 1 } << (id % sizeof(size_t)));
    }

    void ControlBlock::resetDirty()
    {
        memset(dirty.data(), 0, dirty.size() * sizeof(size_t));
    }
} // ns replication


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
, replicationControl{std::make_unique<replication::ControlBlock>()}
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

#pragma once

#include "ecs/query.h"
#include "ecs/multiplayer.h"
#include "components/collision.h"


namespace sp::multiplayer {

class TransformReplication : public sp::ecs::ComponentReplicationBase
{
public:
    sp::SparseSet<uint32_t> info;

    void onEntityDestroyed(uint32_t index) override
    {
        info.remove(index);
    }

    void sendAll(sp::io::DataBuffer& packet) override
    {
        for(auto [entity, transform] : sp::ecs::Query<sp::Transform>())
        {
            packet << CMD_ECS_SET_COMPONENT << component_index << entity.getIndex();
            auto p = transform.getPosition();
            auto r = transform.getRotation();
            packet << p.x << p.y << r;
        }
    }

    void update(sp::io::DataBuffer& packet) override
    {
        for(auto [entity, transform] : sp::ecs::Query<sp::Transform>()) {
            if (!info.has(entity.getIndex()) || transform.multiplayer_dirty) {
                info.set(entity.getIndex(), entity.getVersion());
                packet << CMD_ECS_SET_COMPONENT << component_index << entity.getIndex();
                auto p = transform.getPosition();
                auto r = transform.getRotation();
                packet << p.x << p.y << r;
                transform.multiplayer_dirty = false;
                transform.last_send_position = p;
                transform.last_send_rotation = r;
            }
        }
        for(auto [index, data] : info) {
            if (!sp::ecs::Entity::forced(index, data).hasComponent<sp::Transform>()) {
                info.remove(index);
                packet << CMD_ECS_DEL_COMPONENT << component_index << index;
            }
        }
    }

    void receive(sp::ecs::Entity entity, sp::io::DataBuffer& packet) override
    {
        float x, y, r;
        packet >> x >> y >> r;
        auto t = entity.getOrAddComponent<sp::Transform>();
        t.setPosition({x, y});
        t.setRotation(r);
    }

    void remove(sp::ecs::Entity entity) override
    {
        entity.removeComponent<sp::Transform>();
    }
};

}
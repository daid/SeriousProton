#include "multiplayer/collision.h"
#include "ecs/query.h"
#include "ecs/multiplayer.h"
#include "components/collision.h"
#include "engine.h"


namespace sp::multiplayer {

void TransformReplication::onEntityDestroyed(uint32_t index)
{
    info.remove(index);
}

void TransformReplication::sendAll(sp::io::DataBuffer& packet)
{
    for(auto [entity, transform] : sp::ecs::Query<sp::Transform>())
    {
        packet << CMD_ECS_SET_COMPONENT << component_index << entity.getIndex();
        auto p = transform.getPosition();
        auto r = transform.getRotation();
        packet << p.x << p.y << r;
    }
}

void TransformReplication::update(sp::io::DataBuffer& packet)
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
            transform.last_send_time = engine->getElapsedTime();
        }
    }
    for(auto [index, data] : info) {
        if (!sp::ecs::Entity::forced(index, data).hasComponent<sp::Transform>()) {
            info.remove(index);
            packet << CMD_ECS_DEL_COMPONENT << component_index << index;
        }
    }
}

void TransformReplication::receive(sp::ecs::Entity entity, sp::io::DataBuffer& packet)
{
    float x, y, r;
    packet >> x >> y >> r;
    auto t = entity.getOrAddComponent<sp::Transform>();
    t.setPosition({x, y});
    t.setRotation(r);
}

void TransformReplication::remove(sp::ecs::Entity entity)
{
    entity.removeComponent<sp::Transform>();
}

}
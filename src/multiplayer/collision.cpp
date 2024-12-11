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
    for(auto [index, data] : info) {
        if (!sp::ecs::Entity::forced(index, data).hasComponent<sp::Transform>()) {
            info.remove(index);
            packet << CMD_ECS_DEL_COMPONENT << component_index << index;
        }
    }
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
}

void TransformReplication::receive(sp::ecs::Entity entity, sp::io::DataBuffer& packet)
{
    float x, y, r;
    packet >> x >> y >> r;
    auto& t = entity.getOrAddComponent<sp::Transform>();
    t.setPosition({x, y});
    t.setRotation(r);
}

void TransformReplication::remove(sp::ecs::Entity entity)
{
    entity.removeComponent<sp::Transform>();
}

void PhysicsReplication::onEntityDestroyed(uint32_t index)
{
    info.remove(index);
}

void PhysicsReplication::sendAll(sp::io::DataBuffer& packet)
{
    for(auto [entity, physics] : sp::ecs::Query<sp::Physics>())
    {
        packet << CMD_ECS_SET_COMPONENT << component_index << entity.getIndex();
        packet << uint32_t(3) << physics.type << physics.shape << physics.size.x << physics.size.y;
        packet << physics.linear_velocity.x << physics.linear_velocity.y << physics.angular_velocity;
    }
}

void PhysicsReplication::update(sp::io::DataBuffer& packet)
{
    for(auto [index, data] : info) {
        if (!sp::ecs::Entity::forced(index, data.version).hasComponent<sp::Physics>()) {
            info.remove(index);
            packet << CMD_ECS_DEL_COMPONENT << component_index << index;
        }
    }
    for(auto [entity, physics] : sp::ecs::Query<sp::Physics>()) {
        if (!info.has(entity.getIndex()) || physics.multiplayer_dirty) {
            info.set(entity.getIndex(), {entity.getVersion(), physics.linear_velocity, physics.angular_velocity});
            packet << CMD_ECS_SET_COMPONENT << component_index << entity.getIndex();
            packet << uint32_t(3) << physics.type << physics.shape << physics.size.x << physics.size.y;
            packet << physics.linear_velocity.x << physics.linear_velocity.y << physics.angular_velocity;
            physics.multiplayer_dirty = false;
        } else {
            auto& i = info.get(entity.getIndex());
            if (glm::length2(i.velocity - physics.linear_velocity) > 5.0f || glm::length2(i.angular_velocity - physics.angular_velocity) > 5.0f) {
                info.set(entity.getIndex(), {entity.getVersion(), physics.linear_velocity, physics.angular_velocity});
                packet << CMD_ECS_SET_COMPONENT << component_index << entity.getIndex();
                packet << uint32_t(2U) << physics.linear_velocity.x << physics.linear_velocity.y << physics.angular_velocity;
            }
        }
    }
}

void PhysicsReplication::receive(sp::ecs::Entity entity, sp::io::DataBuffer& packet)
{
    auto [flags] = packet.read<uint32_t>();
    auto& p = entity.getOrAddComponent<sp::Physics>();
    if (flags & 1U) {
        auto [type, shape, w, h] = packet.read<sp::Physics::Type, sp::Physics::Shape, float, float>();
        switch(shape) {
        case sp::Physics::Shape::Circle:
            p.setCircle(type, w);
            break;
        case sp::Physics::Shape::Rectangle:
            p.setRectangle(type, {w, h});
            break;
        }
    }
    if (flags & 2U) {
        auto [x, y, a] = packet.read<float, float, float>();
        p.setVelocity({x, y});
        p.setAngularVelocity(a);
    }
}

void PhysicsReplication::remove(sp::ecs::Entity entity)
{
    entity.removeComponent<sp::Transform>();
}

}
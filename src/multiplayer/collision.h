#pragma once

#include "ecs/multiplayer.h"


namespace sp::multiplayer {

class TransformReplication : public sp::ecs::ComponentReplicationBase
{
public:
    sp::SparseSet<uint32_t> info;

    void onEntityDestroyed(uint32_t index) override;
    void sendAll(sp::io::DataBuffer& packet) override;
    void update(sp::io::DataBuffer& packet) override;
    void receive(sp::ecs::Entity entity, sp::io::DataBuffer& packet) override;
    void remove(sp::ecs::Entity entity) override;
};

class PhysicsReplication : public sp::ecs::ComponentReplicationBase
{
public:
    struct Info {
        uint32_t version;
        glm::vec2 velocity;
        float angular_velocity;
    };
    sp::SparseSet<Info> info;

    void onEntityDestroyed(uint32_t index) override;
    void sendAll(sp::io::DataBuffer& packet) override;
    void update(sp::io::DataBuffer& packet) override;
    void receive(sp::ecs::Entity entity, sp::io::DataBuffer& packet) override;
    void remove(sp::ecs::Entity entity) override;
};

}
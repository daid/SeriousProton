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

}
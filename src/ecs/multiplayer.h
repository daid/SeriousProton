#pragma once

#include "io/dataBuffer.h"
#include "ecs/entity.h"
#include "multiplayer_internal.h"

namespace sp::ecs {

class ComponentReplicationBase {
public:
    uint16_t component_index = 0;

    //Called on the server so reused component slots get properly send if an entity is destroyed and reused within the same tick.
    //  And that destroyed entities don't need to send "destroyed component" packets for each component.
    virtual void onEntityDestroyed(uint32_t index) = 0;

    virtual void sendAll(sp::io::DataBuffer& packet) = 0;
    virtual void update(sp::io::DataBuffer& packet) = 0;
    virtual void receive(sp::ecs::Entity entity, sp::io::DataBuffer& packet) = 0;   //Called from client
    virtual void remove(sp::ecs::Entity entity) = 0;                                //Called from client
};

//Simple replication setup that just sends over the whole component each time it is changed.
template<typename T> class ComponentReplication : public ComponentReplicationBase
{
public:
    sp::SparseSet<T> component_copy;

    void onEntityDestroyed(uint32_t index) override
    {
        component_copy.remove(index);
    }

    void sendAll(sp::io::DataBuffer& packet) override
    {
        for(auto [index, data] : sp::ecs::ComponentStorage<T>::storage.sparseset)
        {
            packet << CMD_ECS_SET_COMPONENT << component_index << index << data;
        }
    }

    void update(sp::io::DataBuffer& packet) override
    {
        for(auto [index, data] : sp::ecs::ComponentStorage<T>::storage.sparseset)
        {
            if (!component_copy.has(index) || component_copy.get(index) != data) {
                component_copy.set(index, data);
                packet << CMD_ECS_SET_COMPONENT << component_index << index << data;
            }
        }
        for(auto [index, data] : component_copy)
        {
            if (!sp::ecs::ComponentStorage<T>::storage.sparseset.has(index)) {
                component_copy.remove(index);
                packet << CMD_ECS_DEL_COMPONENT << component_index << index;
            }
        }
    }

    void receive(sp::ecs::Entity entity, sp::io::DataBuffer& packet) override
    {
        T data;
        packet >> data;
        entity.addComponent<T>(data);
    }

    void remove(sp::ecs::Entity entity) override
    {
        entity.removeComponent<T>();
    }
};

class MultiplayerReplication {
public:
    template<typename T> static void registerComponentReplication() {
        auto t = new T();
        t->component_index = list.size();
        list.push_back(t);
    }

private:
    static inline std::vector<ComponentReplicationBase*> list;

    friend class ::GameClient;
    friend class ::GameServer;
};

}

#pragma once

#include "container/sparseset.h"

class GameServer;
template<typename T> class MultiplayerECSComponentReplication;
namespace sp::ecs {

class ComponentStorageBase {
public:
    ComponentStorageBase();

private:
    ComponentStorageBase* next = nullptr;
protected:
    static void destroyAll(uint32_t index);
    virtual void destroy(uint32_t index) = 0;

    friend class Entity;
};

template<typename T> class ComponentStorage : public ComponentStorageBase {
    void destroy(uint32_t index) override
    {
        sparseset.remove(index);
    }

    SparseSet<T> sparseset;

    static inline ComponentStorage<T> storage;

    friend class Entity;
    template<class, class...> friend class Query;
    friend class ::MultiplayerECSComponentReplication<T>;
};

}
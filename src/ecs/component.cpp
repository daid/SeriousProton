#include "ecs/component.h"

namespace sp::ecs {

static ComponentStorageBase* all_component_storage = nullptr;

ComponentStorageBase::ComponentStorageBase()
{
    this->next = all_component_storage;
    all_component_storage = this;
}

void ComponentStorageBase::destroyAll(uint32_t index)
{
    for(auto storage = all_component_storage; storage; storage = storage->next)
        storage->destroy(index);
}

}
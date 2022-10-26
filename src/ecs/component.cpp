#include "ecs/component.h"

namespace sp::ecs {

static std::vector<ComponentStorageBase*> all_component_storage;

ComponentStorageBase::ComponentStorageBase()
{
    all_component_storage.push_back(this);
}

void ComponentStorageBase::destroyAll(uint32_t index)
{
    for(auto storage : all_component_storage)
        storage->destroy(index);
}

}
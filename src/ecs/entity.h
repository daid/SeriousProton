#pragma once

#include <stdint.h>
#include <limits>

#include "component.h"


namespace sp::ecs {

// An entity is a lightweight, copyable reference to entity components.
//	Entities have to be created with the create() function, and explicitly destroy()ed.
class Entity final {
public:
	Entity() = default;
	
	static Entity create();
	void destroy();

	explicit operator bool() const;

	template<class T> T* getComponent()
	{
		if (!hasComponent<T>())
			return nullptr;
		return &ComponentStorage<T>::storage.sparseset.get(index);
	}
	template<class T> T& addComponent()
	{
		ComponentStorage<T>::storage.sparseset.set(index, {});
		return ComponentStorage<T>::storage.sparseset.get(index);
	}
	template<class T, class... ARGS> T& addComponent(ARGS&&... args)
	{
		ComponentStorage<T>::storage.sparseset.set(index, T{std::forward<ARGS>(args)...});
		return ComponentStorage<T>::storage.sparseset.get(index);
	}
	template<class T> bool hasComponent()
	{
		return ComponentStorage<T>::storage.sparseset.has(index);
	}

private:
	static Entity fromIndex(uint32_t index);

	uint32_t index = std::numeric_limits<uint32_t>::max();
	uint32_t version = std::numeric_limits<uint32_t>::max();

	template<class, class...> friend class Query;
};

}
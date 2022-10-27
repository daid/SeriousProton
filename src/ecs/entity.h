#pragma once

#include <stdint.h>
#include <limits>

#include "component.h"


class MultiplayerObject;
class GameServer;
class GameClient;
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

	uint32_t index = std::numeric_limits<uint32_t>::max();
	uint32_t version = std::numeric_limits<uint32_t>::max();

	static Entity fromIndex(uint32_t index);
	static std::vector<uint32_t> entity_version;
	static std::vector<uint32_t> free_list;

	template<class, class...> friend class Query;

	friend class ::MultiplayerObject;	// We need to be a friend for network replication.
	friend class ::GameServer;	// We need to be a friend for network replication.
	friend class ::GameClient;	// We need to be a friend for network replication.
};

}
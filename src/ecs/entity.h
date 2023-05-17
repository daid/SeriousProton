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
		if (!bool(*this) || !hasComponent<T>())
			return nullptr;
		return &ComponentStorage<T>::storage.sparseset.get(index);
	}
	template<class T> const T* getComponent() const
	{
		if (!bool(*this) || !hasComponent<T>())
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
	template<class T> T& getOrAddComponent()
	{
		if (!hasComponent<T>())
			ComponentStorage<T>::storage.sparseset.set(index, {});
		return ComponentStorage<T>::storage.sparseset.get(index);
	}
	template<class T> bool hasComponent() const
	{
		return ComponentStorage<T>::storage.sparseset.has(index);
	}
	template<class T> void removeComponent()
	{
		ComponentStorage<T>::storage.sparseset.remove(index);
	}

	bool operator==(const Entity& other) const;
	bool operator!=(const Entity& other) const;

	uint32_t getIndex() const { return index; } // You should never need this, but the multiplayer code does need it.
	uint32_t getVersion() const { return version; } // You should never need this, but the multiplayer code does need it.
	static Entity forced(uint32_t index, uint32_t version) { auto e = Entity(); e.index = index; e.version = version; return e; }

	string toString() const;
	static Entity fromString(const string& s);

	static void destroyAllEntities();

	static void dumpDebugInfo();

	static constexpr uint32_t no_index = std::numeric_limits<uint32_t>::max();
private:
	static constexpr uint32_t destroyed_flag = 1 << (std::numeric_limits<uint32_t>::digits - 1);

	uint32_t index = no_index;
	uint32_t version = no_index;

	static Entity fromIndex(uint32_t index);
	static std::vector<uint32_t> entity_version;
	static std::vector<uint32_t> free_list;

	template<class, class...> friend class Query;

	friend class ::MultiplayerObject;	// We need to be a friend for network replication.
	friend class ::GameServer;	// We need to be a friend for network replication.
	friend class ::GameClient;	// We need to be a friend for network replication.
};

}
#include "entity.h"
#include "component.h"
#include "logging.h"
#include <vector>

namespace sp::ecs {

std::vector<uint32_t> Entity::entity_version;
std::vector<uint32_t> Entity::free_list;

Entity Entity::create()
{
	Entity e;
	if (free_list.empty()) {
		e.index = entity_version.size();
		e.version = 0;
		entity_version.push_back(0);
	} else {
		e.index = free_list.back();
		free_list.pop_back();
		entity_version[e.index] &=~destroyed_flag;
		e.version = entity_version[e.index];
	}
	return e;
}

Entity Entity::fromIndex(uint32_t index)
{
	Entity e;
	e.index = index;
	e.version = entity_version[index];
	return e;
}

Entity::operator bool() const
{
	if (index == no_index)
		return false;
    return entity_version[index] == version;
}

bool Entity::operator==(const Entity& other) const {
	auto bt = bool(*this);
	auto bo = bool(other);
	if (bt != bo)
		return false;
	if (!bt)
		return true;
	return index == other.index;
}

bool Entity::operator!=(const Entity& other) const {
	auto bt = bool(*this);
	auto bo = bool(other);
	if (bt != bo)
		return true;
	if (!bt)
		return false;
	return index != other.index;
}

void Entity::destroy()
{
	if (!*this)
		return;
	ComponentStorageBase::destroyAll(index);
	
	// By increasing the version number, everything else will know this entity no longer exists.
	entity_version[index] = (entity_version[index] + 1) | destroyed_flag;
	free_list.push_back(index);
}

string Entity::toString() const
{
	if (!*this)
		return "";
	return string(index) + ":" + string(version);
}

Entity Entity::fromString(const string& s)
{
	auto idx = s.find(':');
	if (idx < 0)
		return {};
	return forced(s.substr(0, idx).toInt(), s.substr(idx + 1).toInt());
}

void Entity::destroyAllEntities()
{
	for(size_t index = 0; index < entity_version.size(); index++) {
		if (!(entity_version[index] & destroyed_flag))
			forced(index, entity_version[index]).destroy();
	}
}

void Entity::dumpDebugInfo()
{
	LOG(Debug, "Entity count:", entity_version.size() - free_list.size(), " Free entities:", free_list.size());
}

}

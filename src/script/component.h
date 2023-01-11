#ifndef SP_SCRIPT_COMPONENT
#define SP_SCRIPT_COMPONENT

#include "conversion.h"
#include "environment.h"
#include <functional>


namespace sp::script {

class ComponentRegistry
{
public:
    std::function<int(lua_State*, const char*)> getter;
    std::function<int(lua_State*, const char*)> setter;

    static std::unordered_map<std::string, ComponentRegistry> components;
};

template<typename T> class ComponentHandler
{
public:
    static void name(const char* name) {
        auto L = Environment::getLuaState();
        luaL_newmetatable(L, name);
        lua_pushcfunction(L, [](lua_State* L) {
            auto eptr = lua_touserdata(L, -2);
            if (!eptr)
                return 0;
            auto e = *static_cast<ecs::Entity*>(eptr);
            if (!e) return 0;
            auto ptr = e.getComponent<T>();
            if (!ptr) return 0;
            auto it = members.find(luaL_checkstring(L, -1));
            if (it == members.end()) return 0;
            return it->second.getter(L, *ptr);
        });
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, [](lua_State* L) {
            auto eptr = lua_touserdata(L, -3);
            if (!eptr)
                return 0;
            auto e = *static_cast<ecs::Entity*>(eptr);
            if (!e) return 0;
            auto ptr = e.getComponent<T>();
            if (!ptr) return 0;
            auto key = luaL_checkstring(L, -2);
            auto it = members.find(key);
            if (it == members.end()) return luaL_error(L, "Trying to set unknown component member %s", key);
            return it->second.setter(L, *ptr);
        });
        lua_setfield(L, -2, "__newindex");
        lua_pop(L, 1);
        ComponentRegistry::components[name] = {luaComponentGetter, luaComponentSetter};
    }

    static int luaComponentGetter(lua_State* L, const char* key) {
        auto e = Convert<ecs::Entity>::fromLua(L, -2);
        if (!e.hasComponent<T>()) return 0;
        *static_cast<ecs::Entity*>(lua_newuserdata(L, sizeof(ecs::Entity))) = e;
        luaL_getmetatable(L, key);
        lua_setmetatable(L, -2);
        return 1;
    }

    static int luaComponentSetter(lua_State* L, const char* key) {
        auto e = Convert<ecs::Entity>::fromLua(L, -3);
        if (lua_isnil(L, -1)) {
            e.removeComponent<T>();
        } else if (lua_istable(L, -1)) {
            auto& component = e.addComponent<T>();
            lua_pushnil(L);
            while(lua_next(L, -2)) {
                auto key = luaL_checkstring(L, -2);
                auto it = members.find(key);
                if (it == members.end()) return luaL_error(L, "Trying to set unknown component member %s", key);
                it->second.setter(L, component);
                lua_pop(L, 1);
            }
        } else {
            return luaL_error(L, "Bad assignment to component %s, nil or table expected.", key);
        }
        return 1;
    }

    struct MemberData {
        std::function<int(lua_State*, const T&)> getter;
        std::function<int(lua_State*, T&)> setter;
    };

    static inline std::unordered_map<std::string, MemberData> members;
};

}

#endif//SP_SCRIPT_ENVIRONMENT
#ifndef SP_SCRIPT_COMPONENT
#define SP_SCRIPT_COMPONENT

#include "conversion.h"
#include "environment.h"
#include "string.h"
#include "ecs/query.h"
#include <unordered_map>


namespace sp::script {

class ComponentRegistry
{
public:
    using FuncPtr = int(*)(lua_State*, sp::ecs::Entity, const char*);
    FuncPtr getter;
    FuncPtr setter;
    using QueryFuncPtr = int(*)(lua_State*);
    QueryFuncPtr query;

    static std::unordered_map<std::string, ComponentRegistry> components;
};

namespace detail {
    struct MemberData {
        using GetterPtr = int(*)(lua_State*, const void*);
        using SetterPtr = void(*)(lua_State*, void*);

        GetterPtr getter;
        SetterPtr setter;
    };
    struct IndexedMemberData {
        using GetterPtr = int(*)(lua_State*, const void*, int index);
        using SetterPtr = void(*)(lua_State*, void*, int index);

        GetterPtr getter;
        SetterPtr setter;
    };
}

template<typename T> class ComponentHandler
{
public:
    static void name(const char* name) {
        component_name = name;
        ComponentRegistry::components[name] = {luaComponentGetter, luaComponentSetter, luaComponentQuery};

        auto L = Environment::getLuaState();
        luaL_newmetatable(L, name);
        lua_pushcfunction(L, luaIndex);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, luaNewIndex);
        lua_setfield(L, -2, "__newindex");
        lua_pushcfunction(L, luaPairs);
        lua_setfield(L, -2, "__pairs");
        lua_pushcfunction(L, [](lua_State* L) {
            if (!array_count_func) luaL_error(L, "Tried to get length of component %s that has no array", component_name);
            auto ptr = luaToComponent(L, -1);
            if (!ptr) return 0;
            lua_pushinteger(L, array_count_func(*ptr));
            return 1;
        });
        lua_setfield(L, -2, "__len");
        lua_pushstring(L, "sandboxed");
        lua_setfield(L, -2, "__metatable");
        lua_pop(L, 1);
        
        array_metatable_name = name + string("_array");
        luaL_newmetatable(L, array_metatable_name.c_str());
        lua_pushcfunction(L, [](lua_State* L) {
            IndexedComponent* icptr = static_cast<IndexedComponent*>(lua_touserdata(L, -2));
            if (!icptr) return 0;
            if (!icptr->entity) return 0;
            auto ptr = icptr->entity.template getComponent<T>();
            if (!ptr) return 0;
            if (array_count_func(*ptr) <= icptr->index) return 0;
            auto key = luaL_checkstring(L, -1);
            auto it = indexed_members.find(key);
            if (it == indexed_members.end()) return 0;
            return it->second.getter(L, ptr, icptr->index);
        });
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, [](lua_State* L) {
            IndexedComponent* icptr = static_cast<IndexedComponent*>(lua_touserdata(L, -3));
            if (!icptr) return 0;
            if (!icptr->entity) return 0;
            auto ptr = icptr->entity.template getComponent<T>();
            if (!ptr) return 0;
            if (array_count_func(*ptr) <= icptr->index) return luaL_error(L, "Index out of range for assignment on component %s", component_name);
            auto key = luaL_checkstring(L, -2);
            auto it = indexed_members.find(key);
            if (it == indexed_members.end()) return luaL_error(L, "Trying to set unknown component %s member %s", component_name, key);
            it->second.setter(L, ptr, icptr->index);
            return 0;
        });
        lua_setfield(L, -2, "__newindex");
        lua_pushcfunction(L, [](lua_State* L) {
            IndexedComponent* icptr = static_cast<IndexedComponent*>(lua_touserdata(L, -1));
            if (!icptr) return 0;
            if (!icptr->entity) return 0;
            auto ptr = icptr->entity.template getComponent<T>();
            if (!ptr) return 0;
            if (array_count_func(*ptr) <= icptr->index) return luaL_error(L, "Index out of range for pairs on component %s", component_name);

            lua_newtable(L);
            int tbl = lua_gettop(L);
            for (auto mem : indexed_members) {
                mem.second.getter(L, ptr, icptr->index);
                lua_setfield(L, tbl, mem.first.c_str());
            }
            lua_settop(L, tbl);
            lua_getglobal(L, "next");
            lua_rotate(L, 2, -1);
            lua_pushnil(L);
            return 3; // next, tbl, nil
        });
        lua_setfield(L, -2, "__pairs");
        lua_pushstring(L, "sandboxed");
        lua_setfield(L, -2, "__metatable");
        lua_pop(L, 1);
    }

    static inline std::unordered_map<std::string, detail::MemberData> members;
    static inline std::unordered_map<std::string, detail::IndexedMemberData> indexed_members;
    using ArrayCountPtr = int(*)(const T&);
    static inline ArrayCountPtr array_count_func = nullptr;
    using ArrayResizePtr = void(*)(T&, int size);
    static inline ArrayResizePtr array_resize_func = nullptr;

private:
    static int luaIndex(lua_State* L) {
        auto eptr = lua_touserdata(L, -2);
        if (!eptr) return 0;
        auto e = *static_cast<ecs::Entity*>(eptr);
        if (!e) return 0;
        auto ptr = e.getComponent<T>();
        if (!ptr) return 0;
        if (array_count_func && lua_isinteger(L, -1)) {
            int index = lua_tointeger(L, -1);
            if (index < 1 || index > array_count_func(*ptr))
                return 0;
            auto ic = static_cast<IndexedComponent*>(lua_newuserdata(L, sizeof(IndexedComponent)));
            ic->entity = e;
            ic->index = index - 1;
            luaL_getmetatable(L, array_metatable_name.c_str());
            lua_setmetatable(L, -2);
            return 1;
        }
        auto key = luaL_checkstring(L, -1);
        auto it = members.find(key);
        if (it == members.end()) return 0;
        return it->second.getter(L, ptr);
    }

    static int luaNewIndex(lua_State* L) {
        auto ptr = luaToComponent(L, -3);
        if (!ptr) return 0;
        if (array_count_func && lua_isinteger(L, -2)) {
            int index = lua_tointeger(L, -2);
            if (index < 1)
                return 0;
            if (lua_isnil(L, -1)) {
                array_resize_func(*ptr, index - 1);
                return 0;
            } else if (lua_istable(L, -1)) {
                if (index > array_count_func(*ptr))
                    array_resize_func(*ptr, index);
                lua_pushnil(L);
                while(lua_next(L, -2)) {
                    auto key = luaL_checkstring(L, -2);
                    auto it = indexed_members.find(key);
                    if (it == indexed_members.end()) return luaL_error(L, "Trying to set unknown component %s member %s", component_name, key);
                    it->second.setter(L, ptr, index - 1);
                    lua_pop(L, 1);
                }
            } else {
                return luaL_error(L, "Bad assignment to component %s, nil or table expected.", component_name);
            }
            return 0;
        }
        auto key = luaL_checkstring(L, -2);
        auto it = members.find(key);
        if (it == members.end()) return luaL_error(L, "Trying to set unknown component %s member %s", component_name, key);
        it->second.setter(L, ptr);
        return 0;
    }

    static int luaPairs(lua_State* L) {
        auto eptr = lua_touserdata(L, -1);
        if (!eptr) return 0;
        auto e = *static_cast<ecs::Entity*>(eptr);
        if (!e) return 0;
        auto ptr = e.getComponent<T>();
        if (!ptr) return 0;

        lua_newtable(L);
        int tbl = lua_gettop(L);
        if (array_count_func) {
            int count = array_count_func(*ptr);
            for (int i = 0; i < count; i++) {
                auto ic = static_cast<IndexedComponent*>(lua_newuserdata(L, sizeof(IndexedComponent)));
                ic->entity = e;
                ic->index = i;
                luaL_getmetatable(L, array_metatable_name.c_str());
                lua_setmetatable(L, -2);
                lua_seti(L, tbl, i + 1);
            }
        }
        for (auto mem : members) {
            mem.second.getter(L, ptr);
            lua_setfield(L, tbl, mem.first.c_str());
        }

        lua_settop(L, tbl);
        lua_getglobal(L, "next");
        lua_rotate(L, 2, -1);
        lua_pushnil(L);
        return 3; // next, tbl, nil
    }

    static T* luaToComponent(lua_State* L, int index) {
        auto eptr = lua_touserdata(L, index);
        if (!eptr) return nullptr;
        auto e = *static_cast<ecs::Entity*>(eptr);
        if (!e) return nullptr;
        return e.getComponent<T>();
    }

    static int luaComponentGetter(lua_State* L, sp::ecs::Entity e, const char* key) {
        if (!e.hasComponent<T>()) return 0;
        *static_cast<ecs::Entity*>(lua_newuserdata(L, sizeof(ecs::Entity))) = e;
        luaL_getmetatable(L, key);
        lua_setmetatable(L, -2);
        return 1;
    }

    static int luaComponentSetter(lua_State* L, sp::ecs::Entity e, const char* key) {
        if (lua_isnil(L, -1)) {
            e.removeComponent<T>();
        } else if (lua_istable(L, -1)) {
            auto& component = e.getOrAddComponent<T>();
            lua_pushnil(L);
            while(lua_next(L, -2)) {
                if (array_count_func && lua_isinteger(L, -2)) {
                    int index = lua_tointeger(L, -2) - 1;
                    if (index < 0) luaL_error(L, "Cannot assign indexes below 1 on component %s", component_name);
                    if (array_count_func(component) < index + 1)
                        array_resize_func(component, index + 1);
                    luaL_checktype(L, -1, LUA_TTABLE);
                    lua_pushnil(L);
                    while(lua_next(L, -2)) {
                        auto key = luaL_checkstring(L, -2);
                        auto it = indexed_members.find(key);
                        if (it == indexed_members.end()) return luaL_error(L, "Trying to set unknown component %s member %s", component_name, key);
                        it->second.setter(L, &component, index);
                        lua_pop(L, 1);
                    }
                } else {
                    auto key = luaL_checkstring(L, -2);
                    auto it = members.find(key);
                    if (it == members.end()) return luaL_error(L, "Trying to set unknown component %s member %s", component_name, key);
                    it->second.setter(L, &component);
                }
                lua_pop(L, 1);
            }
        } else {
            return luaL_error(L, "Bad assignment to component %s member %s, nil or table expected.", component_name, key);
        }
        return 0;
    }

    static int luaComponentQuery(lua_State* L)
    {
        lua_newtable(L);
        int index = 1;
        for(auto [e, comp] : sp::ecs::Query<T>()) {
            Convert<sp::ecs::Entity>::toLua(L, e);
            lua_rawseti(L, -2, index);
            index++;
        }
        return 1;
    }

    static inline const char* component_name;
    static inline string array_metatable_name;
    struct IndexedComponent {
        sp::ecs::Entity entity;
        int index;
    };
};

}

#endif//SP_SCRIPT_ENVIRONMENT

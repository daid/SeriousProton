#ifndef SP_SCRIPT_COMPONENT
#define SP_SCRIPT_COMPONENT

#include "conversion.h"
#include "environment.h"
#include "string.h"


namespace sp::script {

class ComponentRegistry
{
public:
    using FuncPtr = int(*)(lua_State*, const char*);
    FuncPtr getter;
    FuncPtr setter;

    static std::unordered_map<std::string, ComponentRegistry> components;
};

template<typename T> class ComponentHandler
{
public:
    static void name(const char* name) {
        ComponentRegistry::components[name] = {luaComponentGetter, luaComponentSetter};

        auto L = Environment::getLuaState();
        luaL_newmetatable(L, name);
        lua_pushcfunction(L, luaIndex);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, luaNewIndex);
        lua_setfield(L, -2, "__newindex");
        lua_pushcfunction(L, [](lua_State* L) {
            if (!array_count_func) luaL_error(L, "Tried to get length of component that has no array");
            auto ptr = luaToComponent(L, -1);
            if (!ptr) return 0;
            lua_pushinteger(L, array_count_func(*ptr));
            return 1;
        });
        lua_setfield(L, -2, "__len");
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
            return it->second.getter(L, *ptr, icptr->index);
        });
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, [](lua_State* L) {
            IndexedComponent* icptr = static_cast<IndexedComponent*>(lua_touserdata(L, -3));
            if (!icptr) return 0;
            if (!icptr->entity) return 0;
            auto ptr = icptr->entity.template getComponent<T>();
            if (!ptr) return 0;
            if (array_count_func(*ptr) <= icptr->index) return luaL_error(L, "Index out of range for assignment");
            auto key = luaL_checkstring(L, -2);
            auto it = indexed_members.find(key);
            if (it == indexed_members.end()) return luaL_error(L, "Trying to set unknown component member %s", key);
            it->second.setter(L, *ptr, icptr->index);
            return 0;
        });
        lua_setfield(L, -2, "__newindex");
        lua_pop(L, 1);
    }

    struct MemberData {
        using GetterPtr = int(*)(lua_State*, const T&);
        using SetterPtr = void(*)(lua_State*, T&);

        GetterPtr getter;
        SetterPtr setter;
    };
    struct IndexedMemberData {
        using GetterPtr = int(*)(lua_State*, const T&, int index);
        using SetterPtr = void(*)(lua_State*, T&, int index);

        GetterPtr getter;
        SetterPtr setter;
    };

    static inline std::unordered_map<std::string, MemberData> members;
    static inline std::unordered_map<std::string, IndexedMemberData> indexed_members;
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
        return it->second.getter(L, *ptr);
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
                    if (it == indexed_members.end()) return luaL_error(L, "Trying to set unknown component member %s", key);
                    it->second.setter(L, *ptr, index - 1);
                    lua_pop(L, 1);
                }
            } else {
                return luaL_error(L, "Bad assignment to component, nil or table expected.");
            }
            return 0;
        }
        auto key = luaL_checkstring(L, -2);
        auto it = members.find(key);
        if (it == members.end()) return luaL_error(L, "Trying to set unknown component member %s", key);
        it->second.setter(L, *ptr);
        return 0;
    }

    static T* luaToComponent(lua_State* L, int index) {
        auto eptr = lua_touserdata(L, index);
        if (!eptr) return nullptr;
        auto e = *static_cast<ecs::Entity*>(eptr);
        if (!e) return nullptr;
        return e.getComponent<T>();
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
                if (array_count_func && lua_isinteger(L, -2)) {
                    int index = lua_tointeger(L, -2) - 1;
                    if (index < 0) luaL_error(L, "Cannot assign indexes below 1");
                    if (array_count_func(component) < index + 1)
                        array_resize_func(component, index + 1);
                    luaL_checktype(L, -1, LUA_TTABLE);
                    lua_pushnil(L);
                    while(lua_next(L, -2)) {
                        auto key = luaL_checkstring(L, -2);
                        auto it = indexed_members.find(key);
                        if (it == indexed_members.end()) return luaL_error(L, "Trying to set unknown component member %s", key);
                        it->second.setter(L, component, index);
                        lua_pop(L, 1);
                    }
                } else {
                    auto key = luaL_checkstring(L, -2);
                    auto it = members.find(key);
                    if (it == members.end()) return luaL_error(L, "Trying to set unknown component member %s", key);
                    it->second.setter(L, component);
                }
                lua_pop(L, 1);
            }
        } else {
            return luaL_error(L, "Bad assignment to component %s, nil or table expected.", key);
        }
        return 0;
    }

    static inline string array_metatable_name;
    struct IndexedComponent {
        sp::ecs::Entity entity;
        int index;
    };
};

}

#endif//SP_SCRIPT_ENVIRONMENT
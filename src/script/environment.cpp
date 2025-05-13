#include "script/environment.h"
#include "script/component.h"
#include <string.h>


namespace sp::script {

std::unordered_map<std::string, ComponentRegistry> ComponentRegistry::components;

class LuaTableComponent {
public:
    LuaTableComponent() {
        lua_newtable(Environment::L);
        lua_rawsetp(Environment::L, LUA_REGISTRYINDEX, this);
    }
    ~LuaTableComponent() {
        lua_pushnil(Environment::L);
        lua_rawsetp(Environment::L, LUA_REGISTRYINDEX, this);
    }
    LuaTableComponent(const LuaTableComponent& other) {
        lua_rawgetp(Environment::L, LUA_REGISTRYINDEX, &other);
        lua_rawsetp(Environment::L, LUA_REGISTRYINDEX, this);
    }
    LuaTableComponent& operator=(const LuaTableComponent& other) {
        if (this == &other)
            return *this;
        lua_rawgetp(Environment::L, LUA_REGISTRYINDEX, &other);
        lua_rawsetp(Environment::L, LUA_REGISTRYINDEX, this);
        return *this;
    }
};


lua_State* Environment::L = nullptr;

Environment::Environment(Environment* parent)
{
    getLuaState();
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "_G");

    for(auto s : {
        "assert", "error", "getmetatable", "ipairs", "next", "pairs", "pcall", "rawequal", "rawlen", "rawget", "rawset", "select", "setmetatable", "tonumber", "tostring", "xpcall", "type", "_VERSION",
        "table", "string", "math"
    }) {
        lua_getglobal(L, s);
        lua_setfield(L, -2, s);
    }

    lua_newtable(L);  /* meta table for the environment, with an __index pointing to the parent environment so we can access it's data. */
    if (parent) {
        lua_pushstring(L, "__index");
        lua_rawgetp(L, LUA_REGISTRYINDEX, parent);
        lua_rawset(L, -3);
    }
    lua_pushstring(L, "sandbox");
    lua_setfield(L, -2, "__metatable");
    lua_setmetatable(L, -2);

    lua_rawsetp(L, LUA_REGISTRYINDEX, this);
}

static int luaEntityIsValid(lua_State* L) {
    auto e = Convert<ecs::Entity>::fromLua(L, 1);
    lua_pushboolean(L, static_cast<bool>(e));
    return 1;
}

static int luaEntityDestroy(lua_State* L) {
    auto e = Convert<ecs::Entity>::fromLua(L, 1);
    e.destroy();
    return 0;
}

static int luaEntityIndex(lua_State* L) {
    auto key = luaL_checkstring(L, -1);
    auto e = Convert<ecs::Entity>::fromLua(L, -2);
    if (strcmp(key, "valid") == 0) {
        lua_pushboolean(L, static_cast<bool>(e));
        return 1;
    }
    if (strcmp(key, "components") == 0) {
        *static_cast<ecs::Entity*>(lua_newuserdata(L, sizeof(ecs::Entity))) = e;
        luaL_getmetatable(L, "entity_components");
        lua_setmetatable(L, -2);
        return 1;
    }
    if (key[0] != '_' && luaL_getmetafield(L, -2, key) != LUA_TNIL) {
        return 1;
    }
    if (!e) return 0;
    //Check if this a value in the entityFunctionTable
    lua_getfield(L, LUA_REGISTRYINDEX, "EFT");
    lua_getfield(L, -1, key);
    if (!lua_isnil(L, -1)) {
        return 1;
    }
    lua_pop(L, 2);

    //Get a value from the LTC
    auto ltc = e.getComponent<LuaTableComponent>();
    if (!ltc) return 0;
    lua_rawgetp(L, LUA_REGISTRYINDEX, ltc);
    lua_pushvalue(L, -2);
    lua_rawget(L, -2);
    return 1;
}

static int luaEntityNewIndex(lua_State* L) {
    auto e = Convert<ecs::Entity>::fromLua(L, -3);
    if (!e) return 0;

    auto key = luaL_checkstring(L, -2);
    if (strcmp("components", key) == 0) {
        if (!lua_istable(L, -1))
            return luaL_error(L, "Assigning to components requires a table");

        lua_pushnil(L);
        while(lua_next(L, -2)) {
            auto component_key = luaL_checkstring(L, -2);
            auto it = ComponentRegistry::components.find(component_key);
            if (it == ComponentRegistry::components.end())
                return luaL_error(L, "Tried to set non-exsisting component %s", component_key);
            it->second.setter(L, e, key);
            lua_pop(L, 1);
        }
        return 0;
    }

    // Store this value in the LTC.
    auto& ltc = e.getOrAddComponent<LuaTableComponent>();
    lua_rawgetp(L, LUA_REGISTRYINDEX, &ltc);
    lua_pushvalue(L, -3);
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
    return 0;
}

static int luaEntityComponentsIndex(lua_State* L) {
    auto eptr = lua_touserdata(L, -2);
    if (!eptr) return 0;
    auto e = *static_cast<ecs::Entity*>(eptr);
    if (!e) return 0;

    auto key = luaL_checkstring(L, -1);
    auto it = ComponentRegistry::components.find(key);
    if (it != ComponentRegistry::components.end()) {
        return it->second.getter(L, e, key);
    }
    return 0;
}

static int luaEntityComponentsNewIndex(lua_State* L) {
    auto eptr = lua_touserdata(L, -3);
    if (!eptr) return 0;
    auto e = *static_cast<ecs::Entity*>(eptr);
    if (!e) return 0;

    auto key = luaL_checkstring(L, -2);
    auto it = ComponentRegistry::components.find(key);
    if (it != ComponentRegistry::components.end()) {
        return it->second.setter(L, e, key);
    }
    return luaL_error(L, "Tried to set non-exsisting component %s", key);
}

static int luaEntityEqual(lua_State* L) {
    auto e1 = Convert<ecs::Entity>::fromLua(L, -2);
    if (!e1) return 0;
    auto e2 = Convert<ecs::Entity>::fromLua(L, -1);
    if (!e2) return 0;

    lua_pushboolean(L, e1 == e2);
    return 1;
}

lua_State* Environment::getLuaState()
{
    if (!L) {
        L = luaL_newstate();

        luaL_requiref(L, "_G", luaopen_base, 1);
        lua_pop(L, 1);
        luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1);
        lua_pop(L, 1);
        luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1);
        lua_pop(L, 1);
        luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1);
        lua_pop(L, 1);

        //Protect the string metatable
        lua_pushliteral(L, "");
        lua_getmetatable(L, -1);
        lua_pushstring(L, "sandboxed");
        lua_setfield(L, -2, "__metatable");
        lua_pop(L, 2);

        lua_newtable(L);
        lua_setfield(L, LUA_REGISTRYINDEX, "EFT");//Entity function table.

        lua_pushlightuserdata(L, nullptr); // Push a "null" entity to set the metatable on
        luaL_newmetatable(L, "entity");
        lua_pushcfunction(L, luaEntityIsValid);
        lua_setfield(L, -2, "isValid");
        lua_pushcfunction(L, luaEntityDestroy);
        lua_setfield(L, -2, "destroy");
        lua_pushcfunction(L, luaEntityIndex);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, luaEntityNewIndex);
        lua_setfield(L, -2, "__newindex");
        lua_pushcfunction(L, luaEntityEqual);
        lua_setfield(L, -2, "__eq");
        lua_pushstring(L, "sandboxed");
        lua_setfield(L, -2, "__metatable");
        lua_setmetatable(L, -2); // Set the metatable on lightuserdata, which applies for all lightuserdata
        lua_pop(L, 1);

        luaL_newmetatable(L, "entity_components");
        lua_pushcfunction(L, luaEntityComponentsIndex);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, luaEntityComponentsNewIndex);
        lua_setfield(L, -2, "__newindex");
        lua_pushstring(L, "sandboxed");
        lua_setfield(L, -2, "__metatable");
        lua_pop(L, 1);
    }
    return L;
}

Environment::~Environment()
{
    lua_pushnil(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, this);
}

bool Environment::isFunction(const string& function_name)
{
    //Try to find our function in the environment table
    lua_rawgetp(L, LUA_REGISTRYINDEX, this);
    lua_getfield(L, -1, function_name.c_str());
    bool result = lua_isfunction(L, -1);
    lua_pop(L, 2);
    return result;
}

int luaErrorHandler(lua_State* L)
{
    const char * msg = lua_tostring(L, -1);
    luaL_traceback(L, L, msg, 2);
    lua_remove(L, -2);
    return 1;
}

}

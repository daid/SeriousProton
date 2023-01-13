#ifndef SP_SCRIPT_CONVERSION
#define SP_SCRIPT_CONVERSION

#include "stringImproved.h"
#include "ecs/entity.h"
#include "lua/lua.hpp"


namespace sp::script {

template<typename T> struct Convert {};
template<> struct Convert<bool> {
    static int toLua(lua_State* L, bool value) { lua_pushboolean(L, value); return 1; }
    static bool fromLua(lua_State* L, int idx) { return lua_toboolean(L, idx); }
};
template<> struct Convert<int> {
    static int toLua(lua_State* L, int value) { lua_pushinteger(L, value); return 1; }
    static int fromLua(lua_State* L, int idx) { return lua_tointeger(L, idx); }
};
template<> struct Convert<float> {
    static int toLua(lua_State* L, float value) { lua_pushnumber(L, value); return 1; }
    static float fromLua(lua_State* L, int idx) { return lua_tonumber(L, idx); }
};
template<> struct Convert<double> {
    static int toLua(lua_State* L, double value) { lua_pushnumber(L, value); return 1; }
    static double fromLua(lua_State* L, int idx) { return lua_tonumber(L, idx); }
};
template<> struct Convert<lua_CFunction> {
    static int toLua(lua_State* L, lua_CFunction value) { lua_pushcfunction(L, value); return 1; }
};
template<> struct Convert<ecs::Entity> {
    static int toLua(lua_State* L, ecs::Entity value) {
        *static_cast<ecs::Entity*>(lua_newuserdata(L, sizeof(ecs::Entity))) = value;
        luaL_getmetatable(L, "entity");
        lua_setmetatable(L, -2);
        return 1;
    }
    static ecs::Entity fromLua(lua_State* L, int idx) {
        auto ptr = luaL_testudata(L, idx, "entity");
        if (!ptr)
            return {};
        return *static_cast<ecs::Entity*>(ptr);
    }
};

template<typename RET, typename... ARGS> struct Convert<RET(*)(ARGS...)> {
    using FT = RET(*)(ARGS...);
    static int toLua(lua_State* L, FT value) {
        auto f = reinterpret_cast<FT*>(lua_newuserdata(L, sizeof(FT)));
        *f = value;
        lua_pushcclosure(L, [](lua_State* L) -> int {
            auto f = *reinterpret_cast<FT*>(lua_touserdata(L, lua_upvalueindex(1)));
            return callHelper(L, f, std::make_index_sequence<sizeof...(ARGS)>());
        }, 1);
        return 1;
    }
private:
    template<std::size_t... N> static int callHelper(lua_State* L, FT f, std::index_sequence<N...>) {
        if constexpr (!std::is_void_v<RET>) {
            auto res = f(Convert<ARGS>::fromLua(L, N + 1)...);
            return Convert<RET>::toLua(L, res);
        } else {
            f(Convert<ARGS>::fromLua(L, N + 1)...);
            return 0;
        }
    }
};

}

#endif//SP_SCRIPT_ENVIRONMENT
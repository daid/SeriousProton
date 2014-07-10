#include "scriptInterfaceMagic.h"

template<> int convert<bool>::returnType(lua_State* L, bool b)
{
    lua_pushboolean(L, b);
    return 1;
}

template<> void convert<const char*>::param(lua_State* L, int& idx, const char*& str)
{
    str = luaL_checkstring(L, idx++);
}

template<> void convert<string>::param(lua_State* L, int& idx, string& str)
{
    str = luaL_checkstring(L, idx++);
}

template<> void convert<bool>::param(lua_State* L, int& idx, bool& b)
{
    b = lua_toboolean(L, idx++);
}

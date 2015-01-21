#include "scriptInterfaceMagic.h"

ScriptClassInfo* scriptClassInfoList;

template<> int convert<bool>::returnType(lua_State* L, bool b)
{
    lua_pushboolean(L, b);
    return 1;
}

template<> int convert<string>::returnType(lua_State* L, string s)
{
    lua_pushstring(L, s.c_str());
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

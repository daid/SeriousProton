#include "lua/lua.hpp"
#include "scriptInterfaceSandbox.h"

// Add or protect a metatable for the provided object
// Input Lua stack: [object]
// Output Lua stack: [object]
void protectLuaMetatable(lua_State* L)
{
    if (!lua_getmetatable(L, -1)) {
        lua_newtable(L);         // [object] [mt]
        lua_pushvalue(L, -1);    // [object] [mt] [mt]
        lua_setmetatable(L, -3); // [object] [mt]
    }

    lua_pushstring(L, "__metatable"); // [object] [mt] "__metatable"
    lua_pushstring(L, "sandbox");     // [object] [mt] "__metatable" "sandbox"
    lua_rawset(L, -3);                // [object] [mt]
    lua_pop(L, 1);                    // [object]
}

#ifndef SCRIPT_INTERFACE_SANDBOX_H
#define SCRIPT_INTERFACE_SANDBOX_H

#include "lua/lua.hpp"

// Add or protect a metatable for the provided object such that the metatable can't be read or changed from inside the sandbox.
// Input Lua stack: [object]
// Output Lua stack: [object]
void protectLuaMetatable(lua_State* L);

#endif // SCRIPT_INTERFACE_SANDBOX_H

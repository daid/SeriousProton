#ifndef SCRIPT_INTERFACE_SANDBOX_H
#define SCRIPT_INTERFACE_SANDBOX_H

#include "lua/lua.hpp"

// Add or protect a metatable for the provided object such that the metatable can't be read or changed from inside the sandbox.
// Input Lua stack: [object]
// Output Lua stack: [object]
void protectLuaMetatable(lua_State* L);

// Create an editable proxy for the provided table.
// The proxy table itself will be editable, but its metatable will be protected and the input table will not be directly accessible.
// Input Lua stack: [table]
// Output Lua stack: [proxy]
void makeEditableLuaProxy(lua_State* L);

// Create a read-only proxy for the provided table.
// The proxy table will be empty and uneditable, its metatable will be protected, and the input table will not be directly accessible.
// Input Lua stack: [table]
// Output Lua stack: [proxy]
void makeReadonlyLuaProxy(lua_State* L);

#endif // SCRIPT_INTERFACE_SANDBOX_H

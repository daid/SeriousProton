#include "lua/lua.hpp"
#include "scriptInterfaceSandbox.h"

int reject_write(lua_State* L) {
    lua_pop(L, lua_gettop(L));
    lua_pushstring(L, "cannot write to sandbox-external library");
    lua_error(L);
    return 0;
}

// Add or protect a metatable, and add that metatable to the stack.
// Input Lua stack: [object]
// Output Lua stack: [object] [mt]
void protectGetLuaMetatable(lua_State* L)
{
    if (!lua_getmetatable(L, -1)) {
        lua_newtable(L);         // [object] [mt]
        lua_pushvalue(L, -1);    // [object] [mt] [mt]
        lua_setmetatable(L, -3); // [object] [mt]
    }

    lua_pushstring(L, "__metatable"); // [object] [mt] "__metatable"
    lua_pushstring(L, "sandbox");     // [object] [mt] "__metatable" "sandbox"
    lua_settable(L, -3);              // [object] [mt]
}

void protectLuaMetatable(lua_State* L)
{
    protectGetLuaMetatable(L); // [object] [mt]
    lua_pop(L, 1);             // [object]
}

// Make the base proxy for a table, and return the proxy and its metatable.
// Input Lua stack: [table]
// Output Lua stack: [proxy] [mt]
void makeLuaProxy(lua_State* L)
{
    lua_newtable(L); // [table] [proxy]

    // Add a protected metatable
    protectGetLuaMetatable(L); // [table] [proxy] [mt]

    // Set the metatable's __index to the original table
    lua_pushstring(L, "__index"); // [table] [proxy] [mt] "__index"
    lua_rotate(L, -4, -1);        // [proxy] [mt] "__index" [table]
    lua_rawset(L, -3);            // [proxy] [mt]
}

void makeEditableLuaProxy(lua_State* L)
{
    makeLuaProxy(L); // [proxy] [mt]
    lua_pop(L, 1);   // [proxy]
}

void makeReadonlyLuaProxy(lua_State* L)
{
    makeLuaProxy(L); // [proxy] [mt]

    // Set the metatable's __newindex to reject writes
    lua_pushstring(L, "__newindex");      // [proxy] [mt] "__newindex"
    lua_pushcclosure(L, reject_write, 0); // [proxy] [mt] "__newindex" [reject_write]
    lua_rawset(L, -3);                    // [proxy] [mt]

    lua_pop(L, 1); // [proxy]
}

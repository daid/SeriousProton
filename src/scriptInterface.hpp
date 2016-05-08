#ifndef _SCRIPTINTERFACE_HPP_
#define _SCRIPTINTERFACE_HPP_

template<> void convert<ScriptSimpleCallback>::param(lua_State* L, int& idx, ScriptSimpleCallback& callback_object)
{
    if (lua_isnil(L, idx))
    {
        //Nil given as parameter to this callback, clear out the callback.
        lua_pushlightuserdata(L, &callback_object);
        lua_pushnil(L);
        lua_settable(L, LUA_REGISTRYINDEX);
        return;
    }
    //Check if the parameter is a function.
    luaL_checktype(L, idx, LUA_TFUNCTION);
    //Check if this function is a lua function, with an reference to the environment.
    //  (We need the environment reference to see if the script to which the function belongs is destroyed when calling the callback)
    if (lua_iscfunction(L, idx))
        luaL_error(L, "Cannot set a binding as callback function.");
    lua_getupvalue(L, idx, 1);
    if (!lua_istable(L, -1))
        luaL_error(L, "??? Upvalue 1 of function is not a table...");
    //Stack is now: [function_environment]
    
    lua_pushlightuserdata(L, &callback_object);
    lua_newtable(L);
    lua_pushstring(L, "script_pointer");

    //Stack is now: [function_environment] [callback object pointer] [table] "script_pointer"
    lua_pushstring(L, "__script_pointer");
    lua_gettable(L, -5);
    if (!lua_islightuserdata(L, -1))
        luaL_error(L, "??? Cannot find reference back to script...");
    //Stack is now: [function_environment] [callback object pointer] [table] "script_pointer" [pointer to script object]
    lua_rawset(L, -3);
    //Stack is now: [function_environment] [callback object pointer] [table]
    lua_pushstring(L, "function");
    lua_pushvalue(L, idx);
    //Stack is now: [function_environment] [callback object pointer] [table] "function" [lua function reference]
    lua_rawset(L, -3);
    
    //Stack is now: [function_environment] [callback object pointer] [table]
    lua_settable(L, LUA_REGISTRYINDEX);
    lua_pop(L, 1);
    
    idx++;
}
#endif /* _SCRIPTINTERFACE_HPP_ */

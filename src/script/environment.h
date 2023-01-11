#ifndef SP_SCRIPT_ENVIRONMENT
#define SP_SCRIPT_ENVIRONMENT

#include "stringImproved.h"
#include "script/conversion.h"
#include "result.h"
#include <lua/lua.hpp>


namespace sp::script {

class Environment : NonCopyable
{
public:
    Environment();
    ~Environment();

    template<typename T> void setGlobal(const string& name, const T& value) {
        //Get the environment table from the registry.
        lua_rawgetp(L, LUA_REGISTRYINDEX, this);
        if (Convert<T>::toLua(L, value) != 1)
            luaL_error(L, "Trying to set global to a type that is not a single value");
        lua_setfield(L, -2, name.c_str());
    }

    template<typename T> Result<T> run(const string& code) {
        int result = luaL_loadbufferx(L, code.c_str(), code.length(), "=[string]", "t");
        if (result) {
            auto res = Result<T>::makeError(luaL_checkstring(L, -1));
            lua_pop(L, 1);
            return res;
        }

        //Get the environment table from the registry.
        lua_rawgetp(L, LUA_REGISTRYINDEX, this);
        //set the environment table it as 1st upvalue
        lua_setupvalue(L, -2, 1);
        
        result = lua_pcall(L, 0, 1, 0);
        if (result)
        {
            auto result = Result<T>::makeError(lua_tostring(L, -1));
            lua_pop(L, 1);
            return result;
        }

        if constexpr (!std::is_void_v<T>) {
            auto return_value = Convert<T>::fromLua(L, -1);
            lua_pop(L, 1);
            return return_value;
        } else {
            lua_pop(L, 1);
            return {};
        }
    }

    template<typename T, typename... ARGS> Result<T> call(const string& function_name, const ARGS&... args) {
        //Try to find our function in the environment table
        lua_rawgetp(L, LUA_REGISTRYINDEX, this);
        lua_getfield(L, -1, function_name.c_str());
        if (!lua_isfunction(L, -1)) {
            lua_pop(L, 2);
            return Result<T>::makeError("Not a function");
        }
        
        int arg_count = (Convert<ARGS>::toLua(Environment::L, args) + ... + 0);
        auto result = lua_pcall(L, arg_count, 1, 0);
        if (result)
        {
            auto result = Result<T>::makeError(lua_tostring(L, -1));
            lua_pop(L, 2);
            return result;
        }

        if constexpr (!std::is_void_v<T>) {
            auto return_value = Convert<T>::fromLua(L, -1);
            lua_pop(L, 2);
            return return_value;
        } else {
            lua_pop(L, 2);
            return {};
        }
    }
private:
    static lua_State* getLuaState();
    static lua_State* L;

    template<typename T> friend class ComponentHandler;
    friend class LuaTableComponent;
    friend class Callback;
};

}

#endif//SP_SCRIPT_ENVIRONMENT

#ifndef SP_SCRIPT_ENVIRONMENT
#define SP_SCRIPT_ENVIRONMENT

#include "stringImproved.h"
#include "script/conversion.h"
#include "result.h"
#include "resources.h"
#include <lua/lua.hpp>


namespace sp::script {

int luaErrorHandler(lua_State* L);

class Environment : NonCopyable
{
public:
    Environment(Environment* parent=nullptr);
    ~Environment();

    void setGlobalFuncWithEnvUpvalue(const string& name, lua_CFunction f) {
        //Get the environment table from the registry.
        lua_rawgetp(L, LUA_REGISTRYINDEX, this);
        lua_pushvalue(L, -1);
        lua_pushcclosure(L, f, 1);
        lua_setfield(L, -2, name.c_str());
        lua_pop(L, 1);
    }

    template<typename T> void setGlobal(const string& name, const T& value) {
        //Get the environment table from the registry.
        lua_rawgetp(L, LUA_REGISTRYINDEX, this);
        if (Convert<T>::toLua(L, value) != 1)
            luaL_error(L, "Trying to set global to a type that is not a single value");
        lua_setfield(L, -2, name.c_str());
        lua_pop(L, 1);
    }

    template<typename T> Result<T> runFile(const string& filename)
    {
        auto stream = getResourceStream(filename);
        if (!stream)
            return Result<T>::makeError("Script file not found: " + filename);
        auto code = stream->readAll();
        stream->destroy();
        stream = nullptr;
        return runImpl<T>(code, "@" + filename);
    }

    template<typename T> Result<T> run(const string& code) {
        return runImpl<T>(code, "=[string]");
    }

    bool isFunction(const string& function_name);

    template<typename T, typename... ARGS> Result<T> call(const string& function_name, const ARGS&... args) {
        lua_pushcfunction(L, luaErrorHandler);
        //Try to find our function in the environment table
        lua_rawgetp(L, LUA_REGISTRYINDEX, this);
        lua_getfield(L, -1, function_name.c_str());
        if (!lua_isfunction(L, -1)) {
            lua_pop(L, 3);
            return Result<T>::makeError("Not a function");
        }
        
        int arg_count = (Convert<ARGS>::toLua(Environment::L, args) + ... + 0);
        auto result = lua_pcall(L, arg_count, 1, -arg_count - 3);
        if (result)
        {
            auto result = Result<T>::makeError(lua_tostring(L, -1));
            lua_pop(L, 3);
            return result;
        }

        if constexpr (!std::is_void_v<T>) {
            auto return_value = Convert<T>::fromLua(L, -1);
            lua_pop(L, 3);
            return return_value;
        } else {
            lua_pop(L, 3);
            return {};
        }
    }

private:
    template<typename T> Result<T> runImpl(const string& code, const string& name="=[string]") {
        int stack_size = lua_gettop(L);
        lua_pushcfunction(L, luaErrorHandler);
        int result = luaL_loadbufferx(L, code.c_str(), code.length(), name.c_str(), "t");
        if (result) {
            auto res = Result<T>::makeError(luaL_checkstring(L, -1));
            lua_settop(L, stack_size);
            return res;
        }

        //Get the environment table from the registry.
        lua_rawgetp(L, LUA_REGISTRYINDEX, this);
        //set the environment table it as 1st upvalue
        lua_setupvalue(L, -2, 1);
        
        int result_count = 1;
        if constexpr (std::is_same_v<T, CaptureAllResults>) {
            result_count = LUA_MULTRET;
        }
        result = lua_pcall(L, 0, result_count, -2);
        if (result) {
            auto result = Result<T>::makeError(lua_tostring(L, -1));
            lua_settop(L, stack_size);
            return result;
        }

        if constexpr (!std::is_void_v<T>) {
            auto return_value = Convert<T>::fromLua(L, -1);
            lua_settop(L, stack_size);
            return return_value;
        } else {
            lua_settop(L, stack_size);
            return {};
        }
    }

    static lua_State* getLuaState();
    static lua_State* L;

    template<typename T> friend class ComponentHandler;
    friend class LuaTableComponent;
    friend class Callback;
    friend class Coroutine;
};

}

#endif//SP_SCRIPT_ENVIRONMENT

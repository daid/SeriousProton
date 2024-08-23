#ifndef SP_SCRIPT_ENVIRONMENT
#define SP_SCRIPT_ENVIRONMENT

#include "stringImproved.h"
#include "script/conversion.h"
#include "result.h"
#include "resources.h"
#include <lua/lua.hpp>
#include <functional>


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

    Result<void> runWithStringResults(const string& code, std::function<void(const string&)> callback) {
        lua_pushcfunction(L, luaErrorHandler);

        // try the code as a return first
        // syntactically `return foo(); bar()` doesn't work in Lua, so if this compiles it was definitely a single statement
        string returncode = "return " + code + ";";

        int result = luaL_loadbufferx(L, returncode.c_str(), returncode.length(), "=[string]", "t");
        if (result) {
            lua_pop(L, 1); // pop the returned error message, try again without `return`
            result = luaL_loadbufferx(L, code.c_str(), code.length(), "=[string]", "t");
            if (result) {
                auto res = Result<void>::makeError(luaL_checkstring(L, -1));
                lua_pop(L, 2);
                return res;
            }
        }

        //Get the environment table from the registry.
        lua_rawgetp(L, LUA_REGISTRYINDEX, this);
        //set the environment table it as 1st upvalue
        lua_setupvalue(L, -2, 1);

        int top = lua_gettop(L);

        result = lua_pcall(L, 0, LUA_MULTRET, -2);
        if (result)
        {
            auto result = Result<void>::makeError(lua_tostring(L, -1));
            lua_pop(L, 2);
            return result;
        }

        // function got popped, return value(s) got pushed; if there's still an element at `top` then it's the first return, and `newtop` is the last return.
        int newtop = lua_gettop(L); // top - 1 + <n return values>

        for (int i = top; i <= newtop; i++) {
            const char *str = luaL_tolstring(L, i, nullptr);
            callback(str);
            lua_pop(L, 1); // pop the string that luaL_tolstring made
        }

        lua_pop(L, 1 + (newtop - top + 1)); // pop errorhandler + return values from stack
        return {};
    }

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
        lua_pushcfunction(L, luaErrorHandler);
        int result = luaL_loadbufferx(L, code.c_str(), code.length(), name.c_str(), "t");
        if (result) {
            auto res = Result<T>::makeError(luaL_checkstring(L, -1));
            lua_pop(L, 2);
            return res;
        }

        //Get the environment table from the registry.
        lua_rawgetp(L, LUA_REGISTRYINDEX, this);
        //set the environment table it as 1st upvalue
        lua_setupvalue(L, -2, 1);
        
        result = lua_pcall(L, 0, 1, -2);
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

    static lua_State* getLuaState();
    static lua_State* L;

    template<typename T> friend class ComponentHandler;
    friend class LuaTableComponent;
    friend class Callback;
};

}

#endif//SP_SCRIPT_ENVIRONMENT

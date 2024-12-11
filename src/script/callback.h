#ifndef SP_SCRIPT_CALLBACK
#define SP_SCRIPT_CALLBACK

#include "result.h"
#include "script/conversion.h"
#include "script/environment.h"


namespace sp::script {

class Callback
{
public:
    Callback();
    Callback(const Callback& other);
    ~Callback();

    Callback& operator=(const Callback& other);

    explicit operator bool();

    template<typename RET, typename... ARGS> Result<RET> call(ARGS... args) {
        //Get this callback from the registry
        lua_rawgetp(Environment::L, LUA_REGISTRYINDEX, this);
        if (!lua_isfunction(Environment::L, -1)) {
            lua_pop(Environment::L, 1);
            return Result<RET>::makeError("Callback not set.");
        }
        //If it exists, push the arguments with it, can run it.
        int arg_count = (Convert<ARGS>::toLua(Environment::L, args) + ... + 0);
        int result = lua_pcall(Environment::L, arg_count, 1, 0);
        if (result) {
            auto ret = Result<RET>::makeError(lua_tostring(Environment::L, -1));
            lua_pop(Environment::L, 1);
            return ret;
        }
        if constexpr (!std::is_void_v<RET>) {
            auto return_value = Convert<RET>::fromLua(Environment::L, -1);
            lua_pop(Environment::L, 1);
            return return_value;
        } else {
            return {};
        }
    }

    // Set a global in the environment of this callback. You generally do not want this.
    //  But I need it for backwards compatibility.
    template<typename T> void setGlobal(const char* key, const T& value) {
        lua_rawgetp(Environment::L, LUA_REGISTRYINDEX, this);
        if (!lua_isfunction(Environment::L, -1)) {
            lua_pop(Environment::L, 1);
            return;
        }
        lua_getupvalue(Environment::L, -1, 1);
        if (!lua_istable(Environment::L, -1)) {
            lua_pop(Environment::L, 2);
            return;
        }
        if (Convert<T>::toLua(Environment::L, value) < 1) {
            lua_pop(Environment::L, 2);
            return;
        }
        lua_setfield(Environment::L, -2, key);
        lua_pop(Environment::L, 2);
    }
};

template<> struct Convert<Callback> {
    static int toLua(lua_State* L, const Callback& callback) {
        lua_rawgetp(L, LUA_REGISTRYINDEX, &callback);
        return 1;
    }
    static Callback fromLua(lua_State* L, int idx) {
        Callback result;
        if (!lua_isnil(L, idx))
            luaL_checktype(L, idx, LUA_TFUNCTION);
        lua_pushvalue(L, idx);
        lua_rawsetp(L, LUA_REGISTRYINDEX, &result);
        return result;
    }
};

}

#endif//SP_SCRIPT_ENVIRONMENT
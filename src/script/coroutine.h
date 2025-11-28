#pragma once

#include <memory>
#include <nonCopyable.h>
#include <result.h>
#include <script/conversion.h>
#include <lua/lua.hpp>

namespace sp::script {

class Coroutine : NonCopyable
{
public:
    Coroutine(lua_State*);
    ~Coroutine();

    /** Resume the coroutine.
     *  Resumes the function that was yielded.
     *  Returns true if the coroutine was still yielded.
     *  Returns false if the coroutine has ended.
     */
    template<typename... ARGS> Result<bool> resume(ARGS... args)
    {
        if (!lua)
            return false;

        int arg_count = (Convert<ARGS>::toLua(lua, args) + ... + 0);
        int nresult = 0;
        int result = lua_resume(lua, nullptr, arg_count, &nresult);
        if (result == LUA_YIELD) {
            return true;
        }
        if (result != LUA_OK)
        {
            auto res = Result<bool>::makeError(lua_tostring(lua, -1));
            lua_pop(lua, 1);
        }
        release();
        return false;
    }

private:
    void release();

    lua_State* lua;
};

typedef std::shared_ptr<Coroutine> CoroutinePtr;
}
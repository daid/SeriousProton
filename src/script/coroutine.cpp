#include "coroutine.h"
#include "environment.h"

namespace sp::script {

Coroutine::Coroutine(lua_State* lua)
: lua(lua)
{
    lua_rawsetp(Environment::L, LUA_REGISTRYINDEX, this);
}

Coroutine::~Coroutine()
{
    release();
}

void Coroutine::release()
{
    if (!lua) return;
    //Remove the coroutine from the registry, so the garbage collector will remove it it.
    lua_pushnil(Environment::L);
    lua_rawsetp(Environment::L, LUA_REGISTRYINDEX, this);
    lua = nullptr;
}

}
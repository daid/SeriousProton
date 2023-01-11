#include "callback.h"
#include "environment.h"

namespace sp::script {

Callback::Callback()
{
}

Callback::Callback(const Callback& other)
{
    lua_rawgetp(Environment::L, LUA_REGISTRYINDEX, &other);
    lua_rawsetp(Environment::L, LUA_REGISTRYINDEX, this);
}

Callback::~Callback()
{
    lua_pushnil(Environment::L);
    lua_rawsetp(Environment::L, LUA_REGISTRYINDEX, this);
}

Callback& Callback::operator=(const Callback& other)
{
    lua_rawgetp(Environment::L, LUA_REGISTRYINDEX, &other);
    lua_rawsetp(Environment::L, LUA_REGISTRYINDEX, this);
    return *this;
}

};

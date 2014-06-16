#include <sys/stat.h>

#include "random.h"
#include "scriptInterface.h"

REGISTER_SCRIPT_CLASS(ScriptObject)
{
    REGISTER_SCRIPT_CLASS_FUNCTION(ScriptObject, run);
}

registerObjectFunctionListItem* registerObjectFunctionListStart;

int lua_random(lua_State* L)
{
    float rMin = luaL_checknumber(L, 1);
    float rMax = luaL_checknumber(L, 2);
    lua_pushnumber(L, random(rMin, rMax));
    return 1;
}

int lua_destroyScript(lua_State* L)
{
    lua_getglobal(L, "__ScriptObjectPointer");
    ScriptObject* obj = static_cast<ScriptObject*>(lua_touserdata(L, -1));
    obj->destroy();
    return 0;
}

ScriptObject::ScriptObject()
{
    L = NULL;
}

ScriptObject::ScriptObject(const char* filename)
{
    L = NULL;
    run(filename);
}

void ScriptObject::run(const char* filename)
{
    if (L == NULL)
    {
        L = luaL_newstate();
    
        lua_pushlightuserdata(L, this);
        lua_setglobal(L, "__ScriptObjectPointer");
        luaL_openlibs(L);
        lua_register(L, "random", lua_random);
        lua_register(L, "destroyScript", lua_destroyScript);
        
        for(registerObjectFunctionListItem* item = registerObjectFunctionListStart; item != NULL; item = item->next)
            item->func(L);
    }

#if AUTO_RELOAD_SCRIPT
    struct stat fileInfo;
    stat(filename, &fileInfo);
    scriptModifyTime = fileInfo.st_mtime;
    lua_pushstring(L, filename);
    lua_setglobal(L, "__ScriptFilename");
#endif
    
    printf("Load: %s\n", filename);
    if (luaL_loadfile(L, filename))
    {
        printf("ERROR(load): %s\n", luaL_checkstring(L, -1));
        destroy();
        return;
    }
    if (lua_pcall(L, 0, 0, 0))
    {
        printf("ERROR(run): %s\n", luaL_checkstring(L, -1));
        destroy();
        return;
    }
    
    lua_getglobal(L, "init");
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 1);
        printf("WARNING(no init function): %s\n", filename);
    }else if (lua_pcall(L, 0, 0, 0))
    {
        printf("ERROR(init): %s\n", luaL_checkstring(L, -1));
    }
}

void ScriptObject::clean()
{
    if (L)
    {
        lua_close(L);
        L = NULL;
    }
}

ScriptObject::~ScriptObject()
{
    clean();
}

void ScriptObject::update(float delta)
{
#if AUTO_RELOAD_SCRIPT
    if (L)
    {
        lua_getglobal(L, "__ScriptFilename");
        const char* filename = luaL_checkstring(L, -1);
        lua_pop(L, 1);
        
        struct stat fileInfo;
        stat(filename, &fileInfo);
        if (scriptModifyTime != fileInfo.st_mtime)
        {
            scriptModifyTime = fileInfo.st_mtime;
            printf("Reload: %s\n", filename);
            if (luaL_loadfile(L, filename))
            {
                printf("ERROR(load): %s\n", luaL_checkstring(L, -1));
                destroy();
                return;
            }
            if (lua_pcall(L, 0, 0, 0))
            {
                printf("ERROR(run): %s\n", luaL_checkstring(L, -1));
                destroy();
                return;
            }
        }
    }
#endif

    if (L)
    {
#ifdef DEBUG
        //Run the garbage collector every update when debugging, to better debug references and leaks.
        lua_gc(L, LUA_GCCOLLECT, 0);
#endif
        lua_getglobal(L, "update");
        lua_pushnumber(L, delta);
        if (lua_pcall(L, 1, 1, 0))
        {
            printf("ERROR(update): %s\n", luaL_checkstring(L, -1));
            destroy();
            return;
        }
        lua_pop(L, 1);
    }
}

void ScriptCallback::operator() ()
{
    if (functionName.size() < 1 || !script)
        return;
    lua_State* L = script->L;
    
    lua_getglobal(L, functionName.c_str());
    if (lua_isnil(L, -1))
    {
        if (luaL_loadstring(L, functionName.c_str()))
        {
            printf("ERROR(%s): %s\n", functionName.c_str(), luaL_checkstring(L, -1));
            lua_pop(L, 1);
            return;
        }
    }
    if (lua_pcall(L, 0, 0, 0))
    {
        printf("ERROR(%s): %s\n", functionName.c_str(), luaL_checkstring(L, -1));
        lua_pop(L, 1);
        return;
    }
}

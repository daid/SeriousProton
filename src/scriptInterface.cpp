#include <sys/stat.h>

#include "random.h"
#include "resources.h"
#include "scriptInterface.h"

REGISTER_SCRIPT_CLASS(ScriptObject)
{
    REGISTER_SCRIPT_CLASS_FUNCTION(ScriptObject, run);
    REGISTER_SCRIPT_CLASS_FUNCTION(ScriptObject, setGlobal);
}

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
    L = luaL_newstate();

    lua_pushlightuserdata(L, this);
    lua_setglobal(L, "__ScriptObjectPointer");
    luaL_openlibs(L);
    lua_register(L, "random", lua_random);
    lua_register(L, "destroyScript", lua_destroyScript);
    
    for(ScriptClassInfo* item = scriptClassInfoList; item != NULL; item = item->next)
        item->register_function(L);
}

ScriptObject::ScriptObject(string filename)
{
    L = NULL;
    run(filename);
}

bool ScriptObject::run(string filename)
{
    if (L == NULL)
    {
        L = luaL_newstate();
    
        lua_pushlightuserdata(L, this);
        lua_setglobal(L, "__ScriptObjectPointer");
        luaL_openlibs(L);
        lua_register(L, "random", lua_random);
        lua_register(L, "destroyScript", lua_destroyScript);
        
        for(ScriptClassInfo* item = scriptClassInfoList; item != NULL; item = item->next)
            item->register_function(L);
    }

#if AUTO_RELOAD_SCRIPT
    struct stat fileInfo;
    stat(filename, &fileInfo);
    scriptModifyTime = fileInfo.st_mtime;
    lua_pushstring(L, filename);
    lua_setglobal(L, "__ScriptFilename");
#endif
    
    LOG(INFO) << "Load script: " << filename;
    P<ResourceStream> stream = getResourceStream(filename);
    if (!stream)
    {
        LOG(ERROR) << "Script not found: " << filename;
        return false;
    }
    
    string filecontents;
    do
    {
        string line = stream->readLine();
        filecontents += line + "\n";
    }while(stream->tell() < stream->getSize());
    
    if (luaL_loadstring(L, filecontents.c_str()))
    {
        LOG(ERROR) << "LUA: load: " << luaL_checkstring(L, -1);
        destroy();
        return false;
    }
    if (lua_pcall(L, 0, 0, 0))
    {
        LOG(ERROR) << "LUA: run: " << luaL_checkstring(L, -1);
        destroy();
        return false;
    }
    
    lua_getglobal(L, "init");
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 1);
        //printf("WARNING(no init function): %s\n", filename);
    }else if (lua_pcall(L, 0, 0, 0))
    {
        LOG(ERROR) << "LUA: init: " << luaL_checkstring(L, -1);
        return false;
    }
    return true;
}

void ScriptObject::setGlobal(string global_name, string value)
{
    if (L)
    {
        lua_pushstring(L, value.c_str());
        lua_setglobal(L, global_name.c_str());
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

void ScriptObject::registerObject(P<PObject> object, string variable_name)
{
    if (!L)
        return;
    string class_name = getScriptClassClassNameFromObject(object);
    if (class_name != "")
    {
        P<PObject>** p = static_cast< P<PObject>** >(lua_newuserdata(L, sizeof(P<PObject>*)));
        *p = new P<PObject>();
        (**p) = object;
        luaL_getmetatable(L, class_name.c_str());
        lua_setmetatable(L, -2);
        lua_setglobal(L, variable_name.c_str());
    }else{
        LOG(ERROR) << "Failed to find class for object " << variable_name;
    }
}

bool ScriptObject::runCode(string code)
{
    if (!L)
        return false;
    if (luaL_dostring(L, code.c_str()))
    {
        LOG(ERROR) << "LUA: " << code << ": " << luaL_checkstring(L, -1);
        lua_pop(L, 1);
        return false;
    }
    lua_settop(L, 0);
    return true;
}

static string luaToJSON(lua_State* L, int index)
{
    if (lua_isnil(L, index) || lua_isnone(L, index))
        return "null";
    if (lua_isboolean(L, index))
        return lua_toboolean(L, index) ? "true" : "false";
    if (lua_isnumber(L, index))
        return string(lua_tonumber(L, index), 3);
    if (lua_isstring(L, index))
        return "\"" + string(lua_tostring(L, index)) + "\"";
    if (lua_istable(L, index))
    {
        string ret = "{";
        lua_pushnil(L);
        bool first = true;
        while(lua_next(L, index) != 0)
        {
            if (first)
                first = false;
            else
                ret += ", ";
            /* uses 'key' (at index -2) and 'value' (at index -1) */
            ret += luaToJSON(L, lua_gettop(L) - 1);
            ret += ": ";
            ret += luaToJSON(L, lua_gettop(L));
            /* removes 'value'; keeps 'key' for next iteration */
            lua_pop(L, 1);
        }
        ret += "}";
        return ret;
    }
    if (lua_isuserdata(L, index))
        return "\"[OBJECT]\"";
    if (lua_isfunction(L, index))
        return "\"[function]\"";
    return "???";
}

bool ScriptObject::runCode(string code, string& json_output)
{
    if (!L)
        return false;
    if (luaL_dostring(L, code.c_str()))
    {
        LOG(ERROR) << "LUA: " << code << ": " << luaL_checkstring(L, -1);
        lua_pop(L, 1);
        return false;
    }
    int nresults = lua_gettop(L);
    json_output = "";
    for(int n=0; n<nresults; n++)
    {
        if (n > 0)
            json_output += ", ";
        json_output += luaToJSON(L, n + 1);
    }
    lua_settop(L, 0);
    return true;
}

bool ScriptObject::callFunction(string name)
{
    if (!L)
        return false;
    lua_getglobal(L, name.c_str());
    if (lua_pcall(L, 0, 0, 0))
    {
        printf("ERROR(%s): %s\n", name.c_str(), luaL_checkstring(L, -1));
        lua_pop(L, 1);
        return false;
    }
    return true;
}

static void runCyclesHook(lua_State *L, lua_Debug *ar)
{
    lua_pushstring(L, "Max execution limit reached. Aborting.");
    lua_error(L);
}

void ScriptObject::setMaxRunCycles(int count)
{
    if (!L)
        return;
    lua_sethook(L, runCyclesHook, LUA_MASKCOUNT, count);
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
                LOG(ERROR) << "LUA: load: " << luaL_checkstring(L, -1));
                destroy();
                return;
            }
            if (lua_pcall(L, 0, 0, 0))
            {
                LOG(ERROR) << "LUA: run: " << luaL_checkstring(L, -1));
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
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 1);
        }else{
            lua_pushnumber(L, delta);
            if (lua_pcall(L, 1, 0, 0))
            {
                LOG(ERROR) << "LUA: update: " << luaL_checkstring(L, -1);
                lua_pop(L, 1);
                return;
            }
        }
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
            LOG(ERROR) << "LUA: " << functionName << ": " << luaL_checkstring(L, -1);
            lua_pop(L, 1);
            return;
        }
    }
    if (lua_pcall(L, 0, 0, 0))
    {
        LOG(ERROR) << "LUA: " << functionName << ": " << luaL_checkstring(L, -1);
        lua_pop(L, 1);
        return;
    }
}

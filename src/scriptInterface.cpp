#include <sys/stat.h>

#include "random.h"
#include "resources.h"
#include "scriptInterface.h"

static int random(lua_State* L)
{
    float rMin = luaL_checknumber(L, 1);
    float rMax = luaL_checknumber(L, 2);
    lua_pushnumber(L, random(rMin, rMax));
    return 1;
}
/// Generate a random number between the min and max value.
REGISTER_SCRIPT_FUNCTION(random);

static int destroyScript(lua_State* L)
{
    ScriptObject* obj = static_cast<ScriptObject*>(lua_touserdata(L, lua_upvalueindex(1)));
    obj->destroy();
    return 0;
}
/// Destroy this script instance. Note that the script will keep running till the end of the current script call.
//REGISTER_SCRIPT_FUNCTION(destroyScript);//Not registered as a normal function, as it needs a reference to the ScriptObject, which is passed as an upvalue.

lua_State* ScriptObject::L = NULL;

ScriptObject::ScriptObject()
{
    max_cycle_count = 0;
    
    createLuaState();
}

ScriptObject::ScriptObject(string filename)
{
    max_cycle_count = 0;
    
    createLuaState();
    run(filename);
}

static const luaL_Reg loadedlibs[] = {
  {"_G", luaopen_base},
//  {LUA_LOADLIBNAME, luaopen_package},
//  {LUA_COLIBNAME, luaopen_coroutine},
  {LUA_TABLIBNAME, luaopen_table},
//  {LUA_IOLIBNAME, luaopen_io},
//  {LUA_OSLIBNAME, luaopen_os},
  {LUA_STRLIBNAME, luaopen_string},
  {LUA_BITLIBNAME, luaopen_bit32},
  {LUA_MATHLIBNAME, luaopen_math},
//  {LUA_DBLIBNAME, luaopen_debug},
  {NULL, NULL}
};

void ScriptObject::createLuaState()
{
    if (L == NULL)
    {
        L = luaL_newstate();

        /* call open functions from 'loadedlibs' and set results to global table */
        for (const luaL_Reg *lib = loadedlibs; lib->func; lib++)
        {
            luaL_requiref(L, lib->name, lib->func, 1);
            lua_pop(L, 1);  /* remove lib */
        }
        
        for(ScriptClassInfo* item = scriptClassInfoList; item != NULL; item = item->next)
            item->register_function(L);
    }

    //Setup a new table as the first upvalue. This will be used as "global" environment for the script. And thus will prevent global namespace polution.
    lua_newtable(L);  /* environment for loaded function */
    
    lua_newtable(L);  /* meta table for the environment, with an __index pointing to the general global table so we can access every global function */
    lua_pushstring(L, "__index");
    lua_pushglobaltable(L);
    lua_rawset(L, -3);
    lua_setmetatable(L, -2);

    //Register the destroyScript function. This needs a reference back to the script object, we pass this as an upvalue.
    lua_pushstring(L, "destroyScript");
    lua_pushlightuserdata(L, this);
    lua_pushcclosure(L, destroyScript, 1);
    lua_rawset(L, -3);

    //Register a pointer to this script object in the environment. So we can get a reference back to this object.
    lua_pushstring(L, "__script_pointer");
    lua_pushlightuserdata(L, this);
    lua_rawset(L, -3);
    
    //Register the environment table for this script object in the registry.
    lua_pushlightuserdata(L, this);
    lua_pushvalue(L, -2);
    lua_settable(L, LUA_REGISTRYINDEX);
    
    //Pop the environment table from the stack
    lua_pop(L, 1);
}

bool ScriptObject::run(string filename)
{
    setCycleLimit();

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
        error_string = luaL_checkstring(L, -1);
        LOG(ERROR) << "LUA: load: " << error_string;
        destroy();
        return false;
    }

    //Get the environment table from the registry.
    lua_pushlightuserdata(L, this);
    lua_gettable(L, LUA_REGISTRYINDEX);
    //set the environment table it as 1st upvalue
    lua_setupvalue(L, -2, 1);
    
    //Call the actual code.
    if (lua_pcall(L, 0, 0, 0))
    {
        error_string = luaL_checkstring(L, -1);
        LOG(ERROR) << "LUA: run: " << error_string;
        lua_pop(L, 1);
        destroy();
        return false;
    }
    
    lua_pushlightuserdata(L, this);
    lua_gettable(L, LUA_REGISTRYINDEX);
    lua_pushstring(L, "init");
    lua_rawget(L, -2);
    lua_remove(L, -2);
    
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 1);
        //LOG(WARNING) << "WARNING(no init function): " << filename;
    }else if (lua_pcall(L, 0, 0, 0))
    {
        error_string = luaL_checkstring(L, -1);
        LOG(ERROR) << "LUA: init: " << error_string;
        lua_pop(L, 1);
        return false;
    }
    return true;
}

void ScriptObject::setVariable(string variable_name, string value)
{
    //Get the environment table from the registry.
    lua_pushlightuserdata(L, this);
    lua_gettable(L, LUA_REGISTRYINDEX);
    
    //Set our variable in this environment table
    lua_pushstring(L, variable_name.c_str());
    lua_pushstring(L, value.c_str());
    lua_settable(L, -3);
    
    //Pop the table
    lua_pop(L, 1);
}

void ScriptObject::registerObject(P<PObject> object, string variable_name)
{
    //Get the environment table from the registry.
    lua_pushlightuserdata(L, this);
    lua_gettable(L, LUA_REGISTRYINDEX);

    //Set our global in this environment table
    lua_pushstring(L, variable_name.c_str());
    
    if (convert< P<PObject> >::returnType(L, object))
    {
        lua_settable(L, -3);
        //Pop the environment table
        lua_pop(L, 1);
    }else{
        LOG(ERROR) << "Failed to find class for object " << variable_name;
        //Need to pop the variable name and the environment table.
        lua_pop(L, 2);
    }
}

bool ScriptObject::runCode(string code)
{
    setCycleLimit();

    if (luaL_loadstring(L, code.c_str()))
    {
        error_string = luaL_checkstring(L, -1);
        LOG(ERROR) << "LUA: " << code << ": " << error_string;
        lua_pop(L, 1);
        return false;
    }

    //Get the environment table from the registry.
    lua_pushlightuserdata(L, this);
    lua_gettable(L, LUA_REGISTRYINDEX);
    //Set it as the first upvalue so it becomes the environment for this call.
    lua_setupvalue(L, -2, 1);
    
    if (lua_pcall(L, 0, LUA_MULTRET, 0))
    {
        error_string = luaL_checkstring(L, -1);
        LOG(ERROR) << "LUA: " << code << ": " << error_string;
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
    if (luaL_loadstring(L, code.c_str()))
    {
        error_string = luaL_checkstring(L, -1);
        LOG(ERROR) << "LUA: " << code << ": " << error_string;
        lua_pop(L, 1);
        return false;
    }

    //Get the environment table from the registry.
    lua_pushlightuserdata(L, this);
    lua_gettable(L, LUA_REGISTRYINDEX);
    //Set it as the first upvalue so it becomes the environment for this call.
    lua_setupvalue(L, -2, 1);
    
    if (lua_pcall(L, 0, LUA_MULTRET, 0))
    {
        error_string = luaL_checkstring(L, -1);
        LOG(ERROR) << "LUA: " << code << ": " << error_string;
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
    setCycleLimit();

    //Get our environment from the registry
    lua_pushlightuserdata(L, this);
    lua_gettable(L, LUA_REGISTRYINDEX);
    //Get the function from the environment
    lua_pushstring(L, name.c_str());
    lua_gettable(L, -2);
    //Call the function
    if (lua_pcall(L, 0, 0, 0))
    {
        error_string = luaL_checkstring(L, -1);
        LOG(ERROR) << "LUA: " << name << ": " << error_string;
        lua_pop(L, 2);
        return false;
    }
    lua_pop(L, 1);
    return true;
}

static void runCyclesHook(lua_State *L, lua_Debug *ar)
{
    lua_pushstring(L, "Max execution limit reached. Aborting.");
    lua_error(L);
}

void ScriptObject::setCycleLimit()
{
    if (max_cycle_count)
        lua_sethook(L, runCyclesHook, LUA_MASKCOUNT, max_cycle_count);
    else
        lua_sethook(L, NULL, 0, 0);
}

void ScriptObject::setMaxRunCycles(int count)
{
    max_cycle_count = count;
}

ScriptObject::~ScriptObject()
{
    //Remove our environment from the registry.
    lua_pushlightuserdata(L, this);
    lua_pushnil(L);
    lua_settable(L, LUA_REGISTRYINDEX);
}

string ScriptObject::getError()
{
    return error_string;
}

void ScriptObject::update(float delta)
{
    setCycleLimit();

    // Get the reference to our environment from the registry.
    lua_pushlightuserdata(L, this);
    lua_gettable(L, LUA_REGISTRYINDEX);
    // Get the update function from the script environment
    lua_pushstring(L, "update");
    lua_gettable(L, -2);
    
    // If it's a function, call it, if not, pop the environment and the function from the stack.
    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 2);
    }else{
        lua_pushnumber(L, delta);
        if (lua_pcall(L, 1, 0, 0))
        {
            LOG(ERROR) << "LUA: update: " << luaL_checkstring(L, -1);
            lua_pop(L, 2);
            return;
        }
        lua_pop(L, 1);
    }
}

void ScriptObject::destroy()
{
    //Remove our environment from the registry.
    lua_pushlightuserdata(L, this);
    lua_pushnil(L);
    lua_settable(L, LUA_REGISTRYINDEX);
    
    Updatable::destroy();
    
    //Running the garbage collector here is good for memory cleaning.
    lua_gc(L, LUA_GCCOLLECT, 0);
}

void ScriptObject::clearDestroyedObjects()
{
#ifdef DEBUG
    //Run the garbage collector every update when debugging, to better debug references and leaks.
    lua_gc(L, LUA_GCCOLLECT, 0);
#endif
    if (lua_gettop(L) != 0)
        LOG(WARNING) << "lua_gettop != 0, could indicate an error in the lua bindings!";

    lua_pushnil(L);
    while (lua_next(L, LUA_REGISTRYINDEX) != 0)
    {
        if (lua_islightuserdata(L, -2) && lua_istable(L, -1))
        {   
            lua_pushstring(L, "__ptr");
            lua_rawget(L, -2);
            if (lua_isuserdata(L, -1))
            {
                P<PObject>** p = static_cast< P<PObject>** >(lua_touserdata(L, -1));
                if (***p == NULL)
                {
                    lua_pushvalue(L, -3);
                    lua_pushnil(L);
                    lua_settable(L, LUA_REGISTRYINDEX);
                }
            }
            lua_pop(L, 1);
        }
        /* removes 'value'; keeps 'key' for next iteration */
        lua_pop(L, 1);
    }
}

ScriptCallback::ScriptCallback()
{
}

ScriptCallback::~ScriptCallback()
{
    lua_State* L = ScriptObject::L;
    
    //Remove ourselves from the registry.
    lua_pushlightuserdata(L, this);
    lua_pushnil(L);
    lua_settable(L, LUA_REGISTRYINDEX);
}

void ScriptCallback::operator() ()
{
    lua_State* L = ScriptObject::L;
    
    lua_pushlightuserdata(L, this);
    lua_gettable(L, LUA_REGISTRYINDEX);
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        return;
    }
    
    lua_pushnil(L);
    while (lua_next(L, -2) != 0)
    {
        if (lua_istable(L, -1))
        {
            lua_pushstring(L, "script_pointer");
            lua_rawget(L, -2);
            //Check if the script pointer is still available as key in the registry. If not, this reference is no longer valid and needs to be removed.
            lua_gettable(L, LUA_REGISTRYINDEX);
            if (!lua_istable(L, -1))
            {
                //Stack is [callback_table] [callback_key] [callback_entry_table] [script_pointer]
                lua_pushvalue(L, -3);
                lua_pushnil(L);
                lua_settable(L, -6);
                lua_pop(L, 1);
            }else{
                lua_pop(L, 1);

                lua_pushstring(L, "function");
                lua_rawget(L, -2);
                
                lua_sethook(L, NULL, 0, 0);
                if (lua_pcall(L, 0, 0, 0))
                {
                    LOG(ERROR) << "Callback function error: " << lua_tostring(L, -1);
                    lua_pop(L, 1);
                }
            }
        }
        /* removes 'value'; keeps 'key' for next iteration */
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
}

#ifndef SCRIPT_INTERFACE_H
#define SCRIPT_INTERFACE_H

#include "scriptInterfaceMagic.h"
#include "stringImproved.h"
#include "Updatable.h"

#define AUTO_RELOAD_SCRIPT 0

class ScriptObject : public Updatable
{
    lua_State* L;
#if AUTO_RELOAD_SCRIPT
    time_t scriptModifyTime;
#endif
public:
    ScriptObject();
    ScriptObject(string filename);
    virtual ~ScriptObject();
    
    void run(string filename);
    void clean();
    void registerObject(P<PObject> object, string variable_name);
    void setGlobal(string global_name, string value);
    void runCode(string code);
    void callFunction(string name);
    virtual void update(float delta);
    
    //Make the ScriptCallback our friend, so we can access the lua_State from the callback class.
    friend class ScriptCallback;
};

class ScriptCallback
{
public:
    P<ScriptObject> script;
    string functionName;

    void operator() ();
    
    void setCallback(const char* functionName);
};

#endif//SCRIPT_INTERFACE_H

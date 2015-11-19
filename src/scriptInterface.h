#ifndef SCRIPT_INTERFACE_H
#define SCRIPT_INTERFACE_H

#include "scriptInterfaceMagic.h"
#include "stringImproved.h"
#include "Updatable.h"

class ScriptObject : public Updatable
{
    static lua_State* L;
    
    int max_cycle_count;
    string error_string;
public:
    ScriptObject();
    ScriptObject(string filename);
    virtual ~ScriptObject();
    
    bool run(string filename);
    void registerObject(P<PObject> object, string variable_name);
    void setVariable(string variable_name, string value);
    bool runCode(string code);
    bool runCode(string code, string& json_output);
    string getError();
    bool callFunction(string name);
    void setMaxRunCycles(int count);
    virtual void update(float delta);

    virtual void destroy();

    static void clearDestroyedObjects();
private:
    void createLuaState();
    void setCycleLimit();
    
    //Make the ScriptCallback our friend, so we can access the lua_State from the callback class.
    friend class ScriptCallback;
};

class ScriptCallback : public sf::NonCopyable
{
public:
    ScriptCallback();
    ~ScriptCallback();

    void operator() ();
};

#endif//SCRIPT_INTERFACE_H

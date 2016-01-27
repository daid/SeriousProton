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
    friend class ScriptSimpleCallback;
};

class ScriptCallback : public sf::NonCopyable
{
public:
    ScriptCallback();
    ~ScriptCallback();

    void operator() ();
};

/**
 Simple callback to the scripting interface.
 This callback only holds a single reference to a script function, is copyable, and can be used as parameter for script binded functions.
*/
class ScriptSimpleCallback
{
public:
    ScriptSimpleCallback();
    ~ScriptSimpleCallback();

    ScriptSimpleCallback(const ScriptSimpleCallback&);
    ScriptSimpleCallback& operator =(const ScriptSimpleCallback&);

    //Returns true if this callback is set and the scriptobject that set it is still valid.
    bool isSet();

    //Call this script function.
    //Returns false when the executed function is no longer available, or returns nil or false.
    // else it will return true.
    bool call();
    
    //Unset this script callback reference.
    void clear();
    
    //Return the script object linked to this callback, if any.
    P<ScriptObject> getScriptObject();
};
template<> void convert<ScriptSimpleCallback>::param(lua_State* L, int& idx, ScriptSimpleCallback& callback);

#endif//SCRIPT_INTERFACE_H

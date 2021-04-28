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
private:

    template<typename ARG, typename... ARGS>
    int pushArgs(lua_State* L, ARG&& arg, ARGS&&... args)
    {
        int headItemsPushedToStack = pushArgs(L, std::forward<ARG>(arg));
        int tailItemsPushedToStack = pushArgs(L, std::forward<ARGS>(args)...);
        return headItemsPushedToStack + tailItemsPushedToStack;
    }
    template<typename T>
    int pushArgs(lua_State* L, T&& thing)
    {
        if (!convert<T>::returnType(L, std::forward<T>(thing)))
        {
            // nothing was pushed on the stack. Push nil explicitly to maintain argument positions
            lua_pushnil(L);
        }
        return 1;
    }
    int pushArgs(lua_State* L)
    {
        return 0;
    }
    
public:
    ScriptSimpleCallback();
    ~ScriptSimpleCallback();

    ScriptSimpleCallback(const ScriptSimpleCallback&);
    ScriptSimpleCallback& operator =(const ScriptSimpleCallback&);

    //Returns true if this callback is set and the scriptobject that set it is still valid.
    bool isSet();

    // Call this script function.
    // Returns a value-less optional when the executed function is no longer available, on nil, or an error occurred.
    // else it will return a set optional with the return value from lua.
    template<typename Return, typename... Args>
    std::conditional_t<std::is_void_v<Return>, void, std::optional<Return>> call(Args&&... args)
    {
        static_assert(std::is_void_v<Return>
            || std::is_arithmetic_v<Return>
            || std::is_enum_v<Return>
            || std::is_convertible_v<std::string, Return>, "return type must be: void, bool, a number, an enum or a string (std::string or SP's).");

        lua_State* L = ScriptObject::L;

        //Get the simple table from the registry. If it's not available, then this callback was never set to anything.
        lua_pushlightuserdata(L, this);
        lua_gettable(L, LUA_REGISTRYINDEX);
        if (!lua_istable(L, -1))
        {
            lua_pop(L, 1);
            if constexpr (std::is_void_v<Return>)
                return;
            else
                return {};
        }
        //Stack is: [table]

        //Push the key "script_pointer" to retrieve the pointer to this script object.
        lua_pushstring(L, "script_pointer");
        lua_rawget(L, -2);
        if (lua_isnil(L, -1))
        {
            //Callback function didn't have an script environment attached to it, so we cannot check if that script still exists.
        }
        else
        {
            //Stack is: [table] [pointer to script object]
            //Check if the script pointer is still available as key in the registry. If not, this reference is no longer valid and needs to be removed.
            lua_gettable(L, LUA_REGISTRYINDEX);
            if (!lua_istable(L, -1))
            {
                lua_pop(L, 2);
                
                if constexpr (std::is_void_v<Return>)
                    return;
                else
                    return {};
            }
        }
        //Remove the script pointer table from the stack, we only needed to check if it exists.
        lua_pop(L, 1);
        //Stack is: [table]

        //Next get our actual function from the stack
        lua_pushstring(L, "function");
        lua_rawget(L, -2);

        int i = pushArgs(L, std::forward<Args>(args)...);
        if (i < 0) // error condition
        {
            lua_pop(L, 2);
            if constexpr (std::is_void_v<Return>)
                return;
            else
                return {};
        }

        //Stack is: [table] [lua function]
        lua_sethook(L, NULL, 0, 0);
        if (lua_pcall(L, i, 1, 0))
        {
            LOG(ERROR) << "Callback function error: " << lua_tostring(L, -1);
            lua_pop(L, 2);
            if constexpr (std::is_void_v<Return>)
                return;
            else
                return {};
        }

        //Stack is: [table] [call result]
        if constexpr (std::is_void_v<Return>)
        {
            if (!lua_isnoneornil(L, -1))
            {
                LOG(DEBUG) << "return value discarded.";
            }
            lua_pop(L, 2);
            return;
        }
        else
        {
            std::optional<Return> result;
            if (!lua_isnoneornil(L, -1))
            {
                if constexpr (std::is_convertible_v<std::string, Return>)
                {
                    if (lua_isstring(L, -1))
                    {
                        result.emplace(lua_tostring(L, -1));
                    }
                    else
                    {
                        LOG(ERROR) << "Unexpected return type (string wanted)";
                    }
                }
                else if constexpr (std::is_same_v<bool, Return>)
                {
                    // No check - it's falsy / truthy values.
                    result.emplace(static_cast<bool>(lua_toboolean(L, -1)));
                }
                else if constexpr (std::is_integral_v<Return> || std::is_enum_v<Return>)
                {
                    if (lua_isinteger(L, -1))
                    {
                        result.emplace(static_cast<Return>(lua_tointeger(L, -1)));
                    }
                    else
                    {
                        LOG(ERROR) << "Unexpected return type (integer wanted)";
                    }
                }
                else if constexpr (std::is_floating_point_v<Return>)
                {
                    if (lua_isnumber(L, -1))
                    {
                        result.emplace(static_cast<Return>(lua_tonumber(L, -1)));
                    }
                    else
                    {
                        LOG(ERROR) << "Unexpected return type (floating point wanted)";
                    }
                }
            }

            lua_pop(L, 2);
            return result;
        }
    }

    //Unset this script callback reference.
    void clear();
    
    //Return the script object linked to this callback, if any.
    P<ScriptObject> getScriptObject();
};
template<> void convert<ScriptSimpleCallback>::param(lua_State* L, int& idx, ScriptSimpleCallback& callback);

#endif//SCRIPT_INTERFACE_H

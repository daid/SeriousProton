#ifndef SCRIPT_INTERFACE_MAGIC_H
#define SCRIPT_INTERFACE_MAGIC_H

/**
    Warning, here be dragons.
    This code links the LUA script engine to C++ classes with a lot of template magic.
    It does automatic parameter conversion, as well as handling "dead" objects.
*/

#include "P.h"
#include "stringImproved.h"
#include "lua/lua.hpp"
#include <SFML/Graphics/Color.hpp>
#include <typeinfo>
#include <optional>

class ScriptClassInfo;

typedef void (*registerObjectFunction)(lua_State* L);
typedef bool (*checkObjectType)(P<PObject> obj);

extern ScriptClassInfo* scriptClassInfoList;
class ScriptClassInfo
{
public:
    string class_name;
    string base_class_name;
    registerObjectFunction register_function;
    checkObjectType check_function;
    ScriptClassInfo* next;
    std::vector<ScriptClassInfo*> child_classes;
    
    ScriptClassInfo(string class_name, string base_class_name, registerObjectFunction register_function, checkObjectType check_function)
    {
        this->class_name = class_name;
        this->base_class_name = base_class_name;
        this->register_function = register_function;
        this->check_function = check_function;
        this->next = scriptClassInfoList;
        scriptClassInfoList = this;
    }
};

string getScriptClassClassNameFromObject(const P<PObject>& object);

template<typename T>
struct convert
{
    // Pick the return *type* based on input:
    // If it's a number-ish format, use it as is.
    // Otherwise use a const reference.
    using return_t = std::conditional_t<
        std::is_arithmetic_v<T> || std::is_enum_v<T>, T,
        std::add_lvalue_reference_t<
            std::add_const_t<T>
        >
    >;
    /* all parameters are by reference. */
    static void param(lua_State* L, int& idx, std::add_lvalue_reference_t<T> t);
    static int returnType(lua_State* L, return_t t);
};

template<typename T>
void convert<T>::param(lua_State* L, int& idx, std::add_lvalue_reference_t<T> t)
{
    //If you get a compile error here, then the function you are trying to register has an parameter that is not handled by the specialized converters, nor
    // by the default number conversion.
    if constexpr (std::is_integral_v<T>)
        t = static_cast<T>(luaL_checkinteger(L, idx++));
    else
        t = static_cast<T>(luaL_checknumber(L, idx++));
}

template<typename T>
int convert<T>::returnType(lua_State* L, return_t t)
{
    if constexpr (!std::is_reference_v<return_t>)
    {
        if constexpr (std::is_integral_v<T>)
            lua_pushinteger(L, t);
        else
            lua_pushnumber(L, t);
        return 1;
    }
    else
    {
        // We got a reference - just delegate to the non-reference implementation.
        return convert<std::remove_reference_t<T>>::returnType(L, t);
    }
    
}
//Specialized template for the bool return type, so we return a lua boolean.
template<> int convert<bool>::returnType(lua_State* L, bool b);
//Specialized template for the string return type, so we return a lua string.
template<> int convert<string>::returnType(lua_State* L, const string& s);

//Have optional parameters, provided they are last arguments of script function
template<typename T>
struct convert<std::optional<T>> //the >> power of c++17
{
    static void param(lua_State* L, int& idx, std::optional<T>& opt_t)
    {
        if (lua_gettop(L) >= idx) //If there are still arguments unwinded
        {
            T res;
            convert<T>::param(L,idx,res);
            opt_t = res;
        }
    }
};

/* Convert parameters to PObject pointers */
template<class T>
struct convert<T*>
{
    static void param(lua_State* L, int& idx, T*& ptr)
    {
        static_assert(std::is_base_of<PObject, T>::value, "T must be a descendant of PObject");

        if (!lua_istable(L, idx))
        {
            const char *msg = lua_pushfstring(L, "Object expected, got %s", luaL_typename(L, idx));
            luaL_argerror(L, idx, msg);
            return;
        }
        lua_pushstring(L, "__ptr");
        lua_gettable(L, idx++);
        
        P<PObject>** p = static_cast< P<PObject>** >(lua_touserdata(L, -1));
        lua_pop(L, 1);
        if (p == NULL)
        {
            ptr = NULL;
            const char *msg = lua_pushfstring(L, "Object expected, got %s", luaL_typename(L, idx-1));
            luaL_argerror(L, idx-1, msg);
            return;
        }
        ptr = dynamic_cast<T*>(***p);
        //printf("ObjParam: %p\n", ptr);
    }
};

template<class T>
struct convert<P<T>>
//TODO: Possible addition, make sure T is a subclass of PObject
{
    static void param(lua_State* L, int& idx, P<T>& ptr)
    {
        if (!lua_istable(L, idx))
        {
            const char *msg = lua_pushfstring(L, "Object expected, got %s", luaL_typename(L, idx));
            luaL_argerror(L, idx, msg);
            return;
        }
        lua_pushstring(L, "__ptr");
        lua_gettable(L, idx++);
        
        P<PObject>** p = static_cast< P<PObject>** >(lua_touserdata(L, -1));
        lua_pop(L, 1);
        if (p == NULL)
        {
            ptr = NULL;
            const char *msg = lua_pushfstring(L, "Object expected, got %s", luaL_typename(L, idx-1));
            luaL_argerror(L, idx-1, msg);
            return;
        }
        ptr = **p;
        //printf("ObjParam: %p\n", ptr);
    }
    
    static int returnType(lua_State* L, const P<T>& object)
    {
        PObject* ptr = *object;
        if (!ptr)
            return 0;
        
        //Try to find this object in the global registry.
        lua_pushlightuserdata(L, ptr);
        lua_gettable(L, LUA_REGISTRYINDEX);
        if (lua_istable(L, -1))
        {
            //Object table already created, so just return a reference to that.
            return 1;
        }
        lua_pop(L, 1);

        string class_name = getScriptClassClassNameFromObject(ptr);
        if (class_name != "")
        {
            lua_newtable(L);

            luaL_getmetatable(L, class_name.c_str());
            lua_setmetatable(L, -2);
            
            lua_pushstring(L, "__ptr");
            P<PObject>** p = static_cast< P<PObject>** >(lua_newuserdata(L, sizeof(P<PObject>*)));
            *p = new P<PObject>();
            (**p) = ptr;
            lua_settable(L, -3);

            lua_pushlightuserdata(L, ptr);
            lua_pushvalue(L, -2);
            lua_settable(L, LUA_REGISTRYINDEX);
            
            return 1;
        }
        return 0;
    }
};
template<class T>
struct convert<PVector<T>>
{
    static int returnType(lua_State* L, const PVector<T>& pvector)
    {
        return convert<std::vector<P<T>>>::returnType(L, pvector);
    }
};

//Specialized template for const char* so we can convert lua strings to C strings. This overrules the general T* template for const char*
template<> void convert<const char*>::param(lua_State* L, int& idx, const char*& str);
template<> void convert<string>::param(lua_State* L, int& idx, string& str);

template<> void convert<bool>::param(lua_State* L, int& idx, bool& b);

template<> void convert<sf::Color>::param(lua_State* L, int& idx, sf::Color& color);

/* Convert parameters to sf::Vector2 objects. */
template<typename T>
struct convert<sf::Vector2<T>>
{
    static void param(lua_State* L, int& idx, sf::Vector2<T>& v)
    {
        convert<T>::param(L, idx, v.x);
        convert<T>::param(L, idx, v.y);
    }
    
    static int returnType(lua_State* L, const sf::Vector2<T>& t)
    {
        auto result = convert<T>::returnType(L, t.x);
        result += convert<T>::returnType(L, t.y);
        return result;
    }
};
/* Convert parameters to sf::Vector3 objects. */
template<typename T>
struct convert<sf::Vector3<T>>
{
    static void param(lua_State* L, int& idx, sf::Vector3<T>& v)
    {
        convert<T>::param(L, idx, v.x);
        convert<T>::param(L, idx, v.y);
        convert<T>::param(L, idx, v.z);
    }
};

/* Convert parameters to std::vector<?> objects. */
template<typename T>
struct convert<std::vector<T>>
{
    static void param(lua_State* L, int& idx, std::vector<T>& v)
    {
        while (idx <= lua_gettop(L))
        {
            convert<T>::param(L, idx, v.emplace_back());
        }
    }

    static int returnType(lua_State* L, const std::vector<T>& vector)
    {
        lua_newtable(L);
        int table_idx = lua_gettop(L);
        int n = 1;
        for(const auto& val : vector)
        {
            int nr_vals = convert<T>::returnType(L, val);
            if (nr_vals > 1)
            {
                LOG(WARNING) << "std::vector<" << typeid(val).name() << "> does not support types that return multiple values. Only the first one is used.";
                lua_pop(L, nr_vals - 1);
            }
            if (nr_vals == 0)
            {
                LOG(WARNING) << "No value added to the stack for std::vector<" << typeid(val).name() << ">. Adding a nil value.";
                lua_pushnil(L);
            }
            lua_seti(L, table_idx, n++);
        }
        return 1;
    }
};
/* Convert parameters to std::map<string, ?> objects. */
template<typename T>
struct convert<std::map<string, T>>
{
    static int returnType(lua_State* L, const std::map<string, T>& map)
    {
        lua_newtable(L);
        int table_idx = lua_gettop(L);
        for(const auto& kv : map)
        {
            int nr_vals = convert<T>::returnType(L, kv.second);
            if (nr_vals > 1)
            {
                LOG(WARNING) << "std::map<string, " << typeid(map).name() << "> does not support types that return multiple values. Only the first one is used.";
                lua_pop(L, nr_vals - 1);
            }
            if (nr_vals == 0)
            {
                LOG(WARNING) << "No value added to the stack for std::map<string, " << typeid(map).name() << ">. Adding a nil value.";
                lua_pushnil(L);
            }
            lua_setfield(L, table_idx, kv.first.c_str());
        }
        return 1;
    }
};

template<class T, typename FuncProto> struct call
{
};

class ScriptCallback;
class ScriptObject;
template<class T> struct call<T, ScriptCallback T::* >
{
    typedef P<PObject>* PT;
    typedef ScriptCallback T::* CallbackProto;
    
    static int setcallbackFunction(lua_State* L)
    {
        //Check if the parameter is a function.
        luaL_checktype(L, 2, LUA_TFUNCTION);
        //Check if this function is a lua function, with an reference to the environment.
        //  (We need the environment reference to see if the script to which the function belongs is destroyed when calling the callback)
        if (lua_iscfunction(L, 2))
            luaL_error(L, "Cannot set a binding as callback function.");
        lua_getupvalue(L, 2, 1);
        if (!lua_istable(L, -1))
            luaL_error(L, "??[setcallbackFunction] Upvalue 1 of function is not a table...");
        lua_pushstring(L, "__script_pointer");
        lua_gettable(L, -2);
        if (!lua_islightuserdata(L, -1))
            luaL_error(L, "??[setcallbackFunction] Cannot find reference back to script...");
        //Stack is now: [function_environment] [pointer]
        
        CallbackProto* callback_ptr = reinterpret_cast<CallbackProto*>(lua_touserdata(L, lua_upvalueindex (1)));
        CallbackProto callback = *callback_ptr;
        T* obj = NULL;
        int idx = 1;
        convert< T* >::param(L, idx, obj);
        if (obj)
        {
            ScriptCallback* callback_object = &((*obj).*(callback));
            lua_pushlightuserdata(L, callback_object);
            lua_gettable(L, LUA_REGISTRYINDEX);
            //Get the table which matches this callback object. If there is no table, create it.
            if (lua_isnil(L, -1))
            {
                lua_pop(L, 1);
                lua_newtable(L);
                lua_pushlightuserdata(L, callback_object);
                lua_pushvalue(L, -2);
                lua_settable(L, LUA_REGISTRYINDEX);
            }
            //The table at [-1] contains a list of callbacks.
            //Stack is now [function_environment] [pointer] [callback_table]
            
            int callback_count = luaL_len(L, -1);
            lua_pushnumber(L, callback_count + 1);
            //Push a new table on the stack, store the pointer to the script object and the function in there.
            lua_newtable(L);
            lua_pushstring(L, "script_pointer");
            lua_pushvalue(L, -5);
            lua_settable(L, -3);
            lua_pushstring(L, "function");
            lua_pushvalue(L, 2);
            lua_settable(L, -3);
            
            //Stack is now [function_environment] [pointer] [callback_table] [callback_index] [this_callback_table]
            //Push the new callback table in the list of callbacks.
            lua_settable(L, -3);
            
            lua_pop(L, 3);
            
            //Put the object on the stack again.
            lua_pushvalue(L, 1);
            return 1;
        }
        return 0;
    }
};

namespace call_details
{
    template<typename T>
    struct defaultReturn
    {
        static int value(lua_State*)
        {
            return 0;
        }
    };

    template<>
    struct defaultReturn<void>
    {
        static int value(lua_State* L)
        {
            lua_pushvalue(L, 1);
            return 1;
        }
    };
    // Iterate over the tuple, and calls convert<> on each of its item.
    template<typename Tuple, std::size_t... Is>
    void convertParametersImpl(lua_State* L, int& idx, Tuple& params, std::index_sequence<Is...>)
    {
        ((convert<std::tuple_element_t<Is, Tuple>>::param(L, idx, std::get<Is>(params))), ...);
    }

    // Helper function to run conversion on a tuple.
    template<typename... Args>
    void convertParameters(lua_State* L, int& idx, std::tuple<Args...>& params)
    {
        convertParametersImpl(L, idx, params, std::index_sequence_for<Args...>{});
    }

    // Execute a function by unpacking a tuple into the function's parameters.
    template<typename T, typename Function, typename Tuple, std::size_t... Is>
    auto executeFunctionImpl(T* obj, Function&& func, Tuple& params, std::index_sequence<Is...>)
    {
        return (obj->*func)(std::get<Is>(params)...);
    }

    // Helper function to execute a function passing a list of arguments into a tuple.
    template<typename T, typename Function, typename... Args>
    auto executeFunction(T* obj, Function&& func, std::tuple<Args...>& params)
    {
        return executeFunctionImpl(obj, std::forward<Function>(func), params, std::index_sequence_for<Args...>{});
    }

    template<typename T, typename Function, typename R, typename... Args>
    auto callFunction(lua_State* L)
    {
        Function* func_ptr = reinterpret_cast<Function*>(lua_touserdata(L, lua_upvalueindex(1)));
        Function func = *func_ptr;
        std::tuple<std::remove_const_t<std::remove_reference_t<Args>>...> params;
        T* obj = nullptr;
        int idx = 1;

        convert<T*>::param(L, idx, obj);
        convertParameters(L, idx, params);
        if (obj)
        {
            if constexpr (!std::is_void_v<R>)
                return convert<R>::returnType(L, executeFunction(obj, func, params));
            else
                executeFunction(obj, func, params);
        }

        return defaultReturn<R>::value(L);
    }
}


template<class T, typename R, typename... Args> struct call<T, R(T::*)(Args...) >
{
    using FuncProto = R(T::*)(Args...);
    
    static int function(lua_State* L)
    {
        return call_details::callFunction<T, FuncProto, R, Args...>(L);
    }
};

template<class T, typename R, typename... Args> struct call<T, R(T::*)(Args...) const>
{
    using FuncProto = R(T::*)(Args...) const;

    static int function(lua_State* L)
    {
        return call_details::callFunction<T, FuncProto, R, Args...>(L);
    }
};

template<class T> class scriptBindObject
{
public:
    static const char* objectTypeName;
    static const char* objectBaseTypeName;
    typedef P<PObject>* PT;

    static int gc_collect(lua_State* L)
    {
        if (!lua_istable(L, -1))
            return 0;
        lua_pushstring(L, "__ptr");
        lua_gettable(L, -2);
        if (lua_isuserdata(L, -1)) //When a subclass is destroyed, it's metatable might call the __gc function on it's sub-metatable. So we can get nil values here, ignore that.
        {
            PT* p = static_cast< PT* >(lua_touserdata(L, -1));
            if (*p)
                delete *p;
        }
        lua_pop(L, 1);
        return 0;
    }
    
    static int create(lua_State* L)
    {
        P<T> ptr = new T();
        
        if (convert< P<T> >::returnType(L, ptr))
            return 1;
        LOG(ERROR) << "Failed to register pointer in script when creating an object...";
        ptr->destroy();
        return 0;
    }

    static int no_create(lua_State* L)
    {
        return 0;
    }
    
    static int isValid(lua_State* L)
    {
        T* obj = NULL;
        int idx = 1;
        convert<T*>::param(L, idx, obj);
        lua_pushboolean(L, obj != NULL);
        return 1;
    }
    
    static int destroy(lua_State* L)
    {
        T* obj = NULL;
        int idx = 1;
        convert<T*>::param(L, idx, obj);
        
        if (obj != NULL)
            obj->destroy();
        return 0;
    }
    
    static void registerObjectCreation(lua_State* L)
    {
        lua_pushcfunction(L, create);
        lua_setglobal(L, objectTypeName);
        registerObject(L);
    }

    static void registerObjectNoCreation(lua_State* L)
    {
        lua_pushcfunction(L, no_create);
        lua_setglobal(L, objectTypeName);
        registerObject(L);
    }
    
    static bool checkObjectType(P<PObject> obj)
    {
        return bool(P<T>(obj));
    }

    static void registerObject(lua_State* L)
    {
        luaL_getmetatable(L, objectTypeName);
        int metatable = lua_gettop(L);
        if (lua_isnil(L, metatable))
        {
            lua_pop(L, 1);
            luaL_newmetatable(L, objectTypeName);
        }
        
        lua_pushstring(L, "__gc");
        lua_pushcfunction(L, gc_collect);
        lua_settable(L, metatable);

        lua_pushstring(L, "__index");
        lua_newtable(L);
        int functionTable = lua_gettop(L);
        registerFunctions(L, functionTable);

        lua_pushstring(L, "isValid");
        lua_pushcclosure(L, isValid, 0);
        lua_settable(L, functionTable);

        lua_pushstring(L, "typeName");
        lua_pushstring(L, objectTypeName);
        lua_settable(L, functionTable);

        lua_pushstring(L, "destroy");
        lua_pushcclosure(L, destroy, 0);
        lua_settable(L, functionTable);

        if (objectBaseTypeName != NULL)
        {
            luaL_getmetatable(L, objectBaseTypeName);
            if (lua_isnil(L, -1))
            {
                lua_pop(L, 1);
                luaL_newmetatable(L, objectBaseTypeName);
            }
            lua_setmetatable(L, functionTable);//Set the metatable of the __index table to the base class
        }

        lua_settable(L, metatable);//Set the __index value in the metatable
        
        lua_pop(L, 1);
    }
    
    static void registerFunctions(lua_State* L, int table);
    
    template<class TT, class FuncProto>
    static void addFunction(lua_State* L, int table, const char* functionName, FuncProto func)
    {
        lua_pushstring(L, functionName);
        FuncProto* ptr = reinterpret_cast<FuncProto*>(lua_newuserdata(L, sizeof(FuncProto)));
        *ptr = func;
        /// If the following line gives a compiler error, then the function you are registering with
        /// REGISTER_SCRIPT_CLASS_FUNCTION has:
        /// * A wrong class given with it (you should give the base class of the function, not a sub class)
        /// * No call handler for the parameter/return type.
        lua_CFunction fptr = &call<TT, FuncProto>::function;
        lua_pushcclosure(L, fptr, 1);
        lua_settable(L, table);
    }

    template<class TT, class FuncProto>
    static void addCallback(lua_State* L, int table, const char* functionName, FuncProto func)
    {
        lua_pushstring(L, functionName);
        FuncProto* ptr = reinterpret_cast<FuncProto*>(lua_newuserdata(L, sizeof(FuncProto)));
        *ptr = func;
        /// If the following line gives a compiler error, then the function you are registering with
        /// REGISTER_SCRIPT_CLASS_FUNCTION has:
        /// * A wrong class given with it (you should give the base class of the function, not a sub class)
        /// * No call handler for the parameter/return type.
        lua_CFunction fptr = &call<TT, FuncProto>::setcallbackFunction;
        lua_pushcclosure(L, fptr, 1);
        lua_settable(L, table);
    }
};

#define REGISTER_SCRIPT_CLASS(T) \
    template <> const char* scriptBindObject<T>::objectTypeName = # T; \
    template <> const char* scriptBindObject<T>::objectBaseTypeName = NULL; \
    ScriptClassInfo scriptClassInfo ## T ( # T , "" , scriptBindObject<T>::registerObjectCreation, scriptBindObject<T>::checkObjectType); \
    template <> void scriptBindObject<T>::registerFunctions(lua_State* L, int table)
#define REGISTER_SCRIPT_CLASS_NO_CREATE(T) \
    template <> const char* scriptBindObject<T>::objectTypeName = # T; \
    template <> const char* scriptBindObject<T>::objectBaseTypeName = NULL; \
    ScriptClassInfo scriptClassInfo ## T ( # T , "" , scriptBindObject<T>::registerObjectNoCreation, scriptBindObject<T>::checkObjectType); \
    template <> void scriptBindObject<T>::registerFunctions(lua_State* L, int table)
#define REGISTER_SCRIPT_SUBCLASS(T, BASE) \
    template <> const char* scriptBindObject<T>::objectTypeName = # T; \
    template <> const char* scriptBindObject<T>::objectBaseTypeName = # BASE; \
    ScriptClassInfo scriptClassInfo ## T ( # T , # BASE , scriptBindObject<T>::registerObjectCreation, scriptBindObject<T>::checkObjectType); \
    template <> void scriptBindObject<T>::registerFunctions(lua_State* L, int table)
#define REGISTER_SCRIPT_SUBCLASS_NO_CREATE(T, BASE) \
    template <> const char* scriptBindObject<T>::objectTypeName = # T; \
    template <> const char* scriptBindObject<T>::objectBaseTypeName = # BASE; \
    ScriptClassInfo scriptClassInfo ## T ( # T , # BASE , scriptBindObject<T>::registerObjectNoCreation, scriptBindObject<T>::checkObjectType); \
    template <> void scriptBindObject<T>::registerFunctions(lua_State* L, int table)
#define REGISTER_SCRIPT_CLASS_FUNCTION(T, F) \
    addFunction<T> (L, table, # F , &T::F)
#define REGISTER_SCRIPT_CLASS_CALLBACK(T, C) \
    addCallback<T> (L, table, # C , &T::C)
#define REGISTER_SCRIPT_FUNCTION(F) \
    static void registerFunctionFunction ## F (lua_State* L) { \
        lua_pushvalue(L, -1); \
        lua_pushcclosure(L, &F, 1); \
        lua_setglobal(L, # F ); \
    } \
    ScriptClassInfo scriptClassInfo ## F ( # F , "" , registerFunctionFunction ## F , NULL );

#endif//SCRIPT_INTERFACE_MAGIC_H

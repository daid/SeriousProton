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

string getScriptClassClassNameFromObject(P<PObject> object);

/* By default convert parameters to numbers types. Should work for int and float types */
template<typename T> struct convert
{
    static void param(lua_State* L, int& idx, T& t)
    {
        //If you get a compile error here, then the function you are trying to register has an parameter that is not handled by the specialized converters, nor
        // by the default number conversion.
        t = luaL_checknumber(L, idx++);
    }
    static int returnType(lua_State* L, T t)
    {
        lua_pushnumber(L, t);
        return 1;
    }
};
//Specialized template for the bool return type, so we return a lua boolean.
template<> int convert<bool>::returnType(lua_State* L, bool b);
//Specialized template for the string return type, so we return a lua string.
template<> int convert<string>::returnType(lua_State* L, string s);

/* Convert parameters to PObject pointers */
template<class T> struct convert<T*>
//TODO: Possible addition, make sure T is a subclass of PObject
{
    static void param(lua_State* L, int& idx, T*& ptr)
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
        ptr = dynamic_cast<T*>(***p);
        //printf("ObjParam: %p\n", ptr);
    }
};

template<class T> struct convert< P<T> >
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
    
    static int returnType(lua_State* L, P<T> object)
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

        string class_name = getScriptClassClassNameFromObject(object);
        if (class_name != "")
        {
            lua_newtable(L);

            luaL_getmetatable(L, class_name.c_str());
            lua_setmetatable(L, -2);
            
            lua_pushstring(L, "__ptr");
            P<PObject>** p = static_cast< P<PObject>** >(lua_newuserdata(L, sizeof(P<PObject>*)));
            *p = new P<PObject>();
            (**p) = object;
            lua_settable(L, -3);

            lua_pushlightuserdata(L, ptr);
            lua_pushvalue(L, -2);
            lua_settable(L, LUA_REGISTRYINDEX);
            
            return 1;
        }
        return 0;
    }
};
template<class T> struct convert< PVector<T> >
{
    static int returnType(lua_State* L, PVector<T> pvector)
    {
        lua_newtable(L);
        int n = 1;
        foreach(T, t, pvector)
        {
            lua_pushnumber(L, n);
            if (convert<P<T> >::returnType(L, t))
            {
                lua_settable(L, -3);
                n++;
            }else{
                lua_pop(L, 1);
            }
        }
        return 1;
    }
};

//Specialized template for const char* so we can convert lua strings to C strings. This overrules the general T* template for const char*
template<> void convert<const char*>::param(lua_State* L, int& idx, const char*& str);
template<> void convert<string>::param(lua_State* L, int& idx, string& str);

template<> void convert<bool>::param(lua_State* L, int& idx, bool& b);

/* Convert parameters to sf::Vector2 objects. */
template<typename T> struct convert<sf::Vector2<T> >
{
    static void param(lua_State* L, int& idx, sf::Vector2<T>& v)
    {
        v.x = luaL_checknumber(L, idx++);
        v.y = luaL_checknumber(L, idx++);
    }
    
    static int returnType(lua_State* L, sf::Vector2<T> t)
    {
        lua_pushnumber(L, t.x);
        lua_pushnumber(L, t.y);
        return 2;
    }
};
/* Convert parameters to sf::Vector3 objects. */
template<typename T> struct convert<sf::Vector3<T> >
{
    static void param(lua_State* L, int& idx, sf::Vector3<T>& v)
    {
        v.x = luaL_checknumber(L, idx++);
        v.y = luaL_checknumber(L, idx++);
        v.z = luaL_checknumber(L, idx++);
    }
};

/* Convert parameters to std::vector<sf::Vector2> objects. */
template<typename T> struct convert<std::vector<sf::Vector2<T> > >
{
    static void param(lua_State* L, int& idx, std::vector<sf::Vector2<T> >& v)
    {
        while(idx < lua_gettop(L))
        {
            T x = luaL_checknumber(L, idx++);
            T y = luaL_checknumber(L, idx++);
            v.push_back(sf::Vector2f(x, y));
        }
    }
};
/* Convert parameters to std::vector<sf::Vector3> objects. */
template<typename T> struct convert<std::vector<sf::Vector3<T> > >
{
    static void param(lua_State* L, int& idx, std::vector<sf::Vector3<T> >& v)
    {
        while(idx < lua_gettop(L))
        {
            T x = luaL_checknumber(L, idx++);
            T y = luaL_checknumber(L, idx++);
            T z = luaL_checknumber(L, idx++);
            v.push_back(sf::Vector3f(x, y, z));
        }
    }
};

template<class T, typename FuncProto> struct call
{
};

template<class T> struct call<T, void(T::*)() >
{
    typedef void(T::*FuncProto)();
    
    static int function(lua_State* L)
    {
        FuncProto* func_ptr = reinterpret_cast<FuncProto*>(lua_touserdata(L, lua_upvalueindex (1)));
        FuncProto func = *func_ptr;
        T* obj = NULL;
        int idx = 1;
        convert<T*>::param(L, idx, obj);
        if (obj)
            (obj->*func)();
        lua_pushvalue(L, 1);
        return 1;
    }
};

template<class T, class R> struct call<T, R(T::*)() >
{
    typedef R(T::*FuncProto)();
    
    static int function(lua_State* L)
    {
        FuncProto* func_ptr = reinterpret_cast<FuncProto*>(lua_touserdata(L, lua_upvalueindex (1)));
        FuncProto func = *func_ptr;
        T* obj = NULL;
        int idx = 1;
        convert<T*>::param(L, idx, obj);
        if (obj)
        {
            R r = (obj->*func)();
            return convert<R>::returnType(L, r);
        }
        return 0;
    }
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
            luaL_error(L, "??? Upvalue 1 of function is not a table...");
        lua_pushstring(L, "__script_pointer");
        lua_gettable(L, -2);
        if (!lua_islightuserdata(L, -1))
            luaL_error(L, "??? Cannot find reference back to script...");
        //Stack is now: [function_environment] [pointer]
        
        CallbackProto* callback_ptr = reinterpret_cast<CallbackProto*>(lua_touserdata(L, lua_upvalueindex (1)));
        CallbackProto callback = *callback_ptr;
        T* obj = NULL;
        int idx = 1;
        convert< T* >::param(L, idx, obj);
        if (obj)
        {
            ScriptCallback* callback_object = &obj->callback;
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
        }
        return 0;
    }
};

template<class T, typename P1> struct call<T, void(T::*)(P1) >
{
    typedef void(T::*FuncProto)(P1 p1);
    
    static int function(lua_State* L)
    {
        FuncProto* func_ptr = reinterpret_cast<FuncProto*>(lua_touserdata(L, lua_upvalueindex (1)));
        FuncProto func = *func_ptr;
        T* obj = NULL;
        P1 p1;
        int idx = 1;
        convert<T*>::param(L, idx, obj);
        convert<P1>::param(L, idx, p1);
        if (obj)
            (obj->*func)(p1);
        lua_pushvalue(L, 1);
        return 1;
    }
};

template<class T, typename R, typename P1> struct call<T, R(T::*)(P1) >
{
    typedef R(T::*FuncProto)(P1 p1);
    
    static int function(lua_State* L)
    {
        FuncProto* func_ptr = reinterpret_cast<FuncProto*>(lua_touserdata(L, lua_upvalueindex (1)));
        FuncProto func = *func_ptr;
        P1 p1;
        T* obj = NULL;
        int idx = 1;
        convert<T*>::param(L, idx, obj);
        convert<P1>::param(L, idx, p1);
        if (obj)
        {
            R r = (obj->*func)(p1);
            return convert<R>::returnType(L, r);
        }
        return 0;
    }
};

template<class T, typename P1, typename P2> struct call<T, void(T::*)(P1, P2) >
{
    typedef void(T::*FuncProto)(P1 p1, P2 p2);
    
    static int function(lua_State* L)
    {
        FuncProto* func_ptr = reinterpret_cast<FuncProto*>(lua_touserdata(L, lua_upvalueindex (1)));
        FuncProto func = *func_ptr;
        P1 p1;
        P2 p2;
        T* obj = NULL;
        int idx = 1;
        convert<T*>::param(L, idx, obj);
        convert<P1>::param(L, idx, p1);
        convert<P2>::param(L, idx, p2);
        if (obj)
            (obj->*func)(p1, p2);
        lua_pushvalue(L, 1);
        return 1;
    }
};

template<class T, typename P1, typename P2, typename P3> struct call<T, void(T::*)(P1, P2, P3) >
{
    typedef void(T::*FuncProto)(P1 p1, P2 p2, P3 p3);
    
    static int function(lua_State* L)
    {
        FuncProto* func_ptr = reinterpret_cast<FuncProto*>(lua_touserdata(L, lua_upvalueindex (1)));
        FuncProto func = *func_ptr;
        P1 p1;
        P2 p2;
        P3 p3;
        T* obj = NULL;
        int idx = 1;
        convert<T*>::param(L, idx, obj);
        convert<P1>::param(L, idx, p1);
        convert<P2>::param(L, idx, p2);
        convert<P3>::param(L, idx, p3);
        if (obj)
            (obj->*func)(p1, p2, p3);
        lua_pushvalue(L, 1);
        return 1;
    }
};

template<class T, typename P1, typename P2, typename P3, typename P4> struct call<T, void(T::*)(P1, P2, P3, P4) >
{
    typedef void(T::*FuncProto)(P1 p1, P2 p2, P3 p3, P4 p4);
    
    static int function(lua_State* L)
    {
        FuncProto* func_ptr = reinterpret_cast<FuncProto*>(lua_touserdata(L, lua_upvalueindex (1)));
        FuncProto func = *func_ptr;
        P1 p1;
        P2 p2;
        P3 p3;
        P4 p4;
        T* obj = NULL;
        int idx = 1;
        convert<T*>::param(L, idx, obj);
        convert<P1>::param(L, idx, p1);
        convert<P2>::param(L, idx, p2);
        convert<P3>::param(L, idx, p3);
        convert<P4>::param(L, idx, p4);
        if (obj)
            (obj->*func)(p1, p2, p3, p4);
        lua_pushvalue(L, 1);
        return 1;
    }
};

template<class T, typename P1, typename P2, typename P3, typename P4, typename P5> struct call<T, void(T::*)(P1, P2, P3, P4, P5) >
{
    typedef void(T::*FuncProto)(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5);
    
    static int function(lua_State* L)
    {
        FuncProto* func_ptr = reinterpret_cast<FuncProto*>(lua_touserdata(L, lua_upvalueindex (1)));
        FuncProto func = *func_ptr;
        P1 p1;
        P2 p2;
        P3 p3;
        P4 p4;
        P5 p5;
        T* obj = NULL;
        int idx = 1;
        convert<T*>::param(L, idx, obj);
        convert<P1>::param(L, idx, p1);
        convert<P2>::param(L, idx, p2);
        convert<P3>::param(L, idx, p3);
        convert<P4>::param(L, idx, p4);
        convert<P5>::param(L, idx, p5);
        if (obj)
            (obj->*func)(p1, p2, p3, p4, p5);
        lua_pushvalue(L, 1);
        return 1;
    }
};

template<class T, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6> struct call<T, void(T::*)(P1, P2, P3, P4, P5, P6) >
{
    typedef void(T::*FuncProto)(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6);
    
    static int function(lua_State* L)
    {
        FuncProto* func_ptr = reinterpret_cast<FuncProto*>(lua_touserdata(L, lua_upvalueindex (1)));
        FuncProto func = *func_ptr;
        P1 p1;
        P2 p2;
        P3 p3;
        P4 p4;
        P5 p5;
        P6 p6;
        T* obj = NULL;
        int idx = 1;
        convert<T*>::param(L, idx, obj);
        convert<P1>::param(L, idx, p1);
        convert<P2>::param(L, idx, p2);
        convert<P3>::param(L, idx, p3);
        convert<P4>::param(L, idx, p4);
        convert<P5>::param(L, idx, p5);
        convert<P6>::param(L, idx, p6);
        if (obj)
            (obj->*func)(p1, p2, p3, p4, p5, p6);
        lua_pushvalue(L, 1);
        return 1;
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
        lua_register(L, # F , &F); \
    }\
    ScriptClassInfo scriptClassInfo ## F ( # F , "" , registerFunctionFunction ## F , NULL );

#endif//SCRIPT_INTERFACE_MAGIC_H

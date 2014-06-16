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

typedef void (*registerObjectFunction)(lua_State* L);

class registerObjectFunctionListItem;
extern registerObjectFunctionListItem* registerObjectFunctionListStart;
class registerObjectFunctionListItem
{
public:
    registerObjectFunction func;
    registerObjectFunctionListItem* next;
    
    registerObjectFunctionListItem(registerObjectFunction func)
    {
        this->func = func;
        this->next = registerObjectFunctionListStart;
        registerObjectFunctionListStart = this;
    }
};

/* By default convert parameters to numbers types. Should work for int and float types */
template<typename T> struct convert
{
    static void param(lua_State* L, int& idx, T& t)
    {
        //If you get a compile error here, then the function you are trying to register has an parameter that is not handled by the specialized converters, nor
        // by the default number conversion.
        t = luaL_checknumber(L, idx++);
    }
    static void returnType(lua_State* L, T t)
    {
        lua_pushnumber(L, t);
    }
};
//Specialized template for the bool return type, so we return a lua boolean.
template<> void convert<bool>::returnType(lua_State* L, bool b);

/* Convert parameters to PObject pointers */
template<class T> struct convert<T*>
//TODO: Possible addition, make sure T is a subclass of PObject
{
    static void param(lua_State* L, int& idx, T*& ptr)
    {
        P<PObject>** p = static_cast< P<PObject>** >(lua_touserdata(L, idx++));
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
        P<PObject>** p = static_cast< P<PObject>** >(lua_touserdata(L, idx++));
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
};

//Specialized template for const char* so we can convert lua strings to C strings. This overrules the general T* template for const char*
template<> void convert<const char*>::param(lua_State* L, int& idx, const char*& str);
template<> void convert<string>::param(lua_State* L, int& idx, string& str);

/* Convert parameters to sf::Vector2 objects. */
template<typename T> struct convert<sf::Vector2<T> >
{
    static void param(lua_State* L, int& idx, sf::Vector2<T>& v)
    {
        v.x = luaL_checknumber(L, idx++);
        v.y = luaL_checknumber(L, idx++);
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

template<class T, typename FuncProto> struct call
{
    typedef P<T>* PT;
};

template<class T> struct call<T, void(T::*)() >
{
    typedef P<PObject>* PT;
    typedef void(T::*FuncProto)();
    
    static int function(lua_State* L)
    {
        FuncProto* func_ptr = reinterpret_cast<FuncProto*>(lua_touserdata(L, lua_upvalueindex (1)));
        FuncProto func = *func_ptr;
        PT* p = static_cast< PT* >(lua_touserdata(L, 1));
        if (p == NULL)
        {
            //Function called without object...
            return 0;
        }
        T* obj = dynamic_cast<T*>(***p);
        if (obj)
            (obj->*func)();
        lua_pushvalue(L, 1);
        return 1;
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
        CallbackProto* callback_ptr = reinterpret_cast<CallbackProto*>(lua_touserdata(L, lua_upvalueindex (1)));
        CallbackProto callback = *callback_ptr;
        PT* p = static_cast< PT* >(lua_touserdata(L, 1));
        if (p == NULL)
        {
            //Function called without object...
            return 0;
        }
        T* obj = dynamic_cast<T*>(***p);
        if (obj)
        {
            lua_getglobal(L, "__ScriptObjectPointer");
            ScriptObject* script = static_cast<ScriptObject*>(lua_touserdata(L, -1));
            lua_pop(L, 1);
            
            (obj->*callback).script = script;
            (obj->*callback).functionName = luaL_checkstring(L, 2);
        }
        lua_pushvalue(L, 1);
        return 1;
    }
};

template<class T, typename P1> struct call<T, void(T::*)(P1) >
{
    typedef P<PObject>* PT;
    typedef void(T::*FuncProto)(P1 p1);
    
    static int function(lua_State* L)
    {
        FuncProto* func_ptr = reinterpret_cast<FuncProto*>(lua_touserdata(L, lua_upvalueindex (1)));
        FuncProto func = *func_ptr;
        PT* p = static_cast< PT* >(lua_touserdata(L, 1));
        T* obj = dynamic_cast<T*>(***p);
        P1 p1;
        int idx = 2;
        convert<P1>::param(L, idx, p1);
        if (obj)
            (obj->*func)(p1);
        lua_pushvalue(L, 1);
        return 1;
    }
};

template<class T, typename R, typename P1> struct call<T, R(T::*)(P1) >
{
    typedef P<PObject>* PT;
    typedef R(T::*FuncProto)(P1 p1);
    
    static int function(lua_State* L)
    {
        FuncProto* func_ptr = reinterpret_cast<FuncProto*>(lua_touserdata(L, lua_upvalueindex (1)));
        FuncProto func = *func_ptr;
        PT* p = static_cast< PT* >(lua_touserdata(L, 1));
        T* obj = dynamic_cast<T*>(***p);
        P1 p1;
        int idx = 2;
        convert<P1>::param(L, idx, p1);
        if (obj)
        {
            R r = (obj->*func)(p1);
            convert<R>::returnType(L, r);
            return 1;
        }
        return 0;
    }
};

template<class T, typename P1, typename P2> struct call<T, void(T::*)(P1, P2) >
{
    typedef P<PObject>* PT;
    typedef void(T::*FuncProto)(P1 p1, P2 p2);
    
    static int function(lua_State* L)
    {
        FuncProto* func_ptr = reinterpret_cast<FuncProto*>(lua_touserdata(L, lua_upvalueindex (1)));
        FuncProto func = *func_ptr;
        PT* p = static_cast< PT* >(lua_touserdata(L, 1));
        T* obj = dynamic_cast<T*>(***p);
        P1 p1;
        P2 p2;
        int idx = 2;
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
    typedef P<PObject>* PT;
    typedef void(T::*FuncProto)(P1 p1, P2 p2, P3 p3);
    
    static int function(lua_State* L)
    {
        FuncProto* func_ptr = reinterpret_cast<FuncProto*>(lua_touserdata(L, lua_upvalueindex (1)));
        FuncProto func = *func_ptr;
        PT* p = static_cast< PT* >(lua_touserdata(L, 1));
        T* obj = dynamic_cast<T*>(***p);
        P1 p1;
        P2 p2;
        P3 p3;
        int idx = 2;
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
    typedef P<PObject>* PT;
    typedef void(T::*FuncProto)(P1 p1, P2 p2, P3 p3, P4 p4);
    
    static int function(lua_State* L)
    {
        FuncProto* func_ptr = reinterpret_cast<FuncProto*>(lua_touserdata(L, lua_upvalueindex (1)));
        FuncProto func = *func_ptr;
        PT* p = static_cast< PT* >(lua_touserdata(L, 1));
        T* obj = dynamic_cast<T*>(***p);
        P1 p1;
        P2 p2;
        P3 p3;
        P4 p4;
        int idx = 2;
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
    typedef P<PObject>* PT;
    typedef void(T::*FuncProto)(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5);
    
    static int function(lua_State* L)
    {
        FuncProto* func_ptr = reinterpret_cast<FuncProto*>(lua_touserdata(L, lua_upvalueindex (1)));
        FuncProto func = *func_ptr;
        PT* p = static_cast< PT* >(lua_touserdata(L, 1));
        T* obj = dynamic_cast<T*>(***p);
        P1 p1;
        P2 p2;
        P3 p3;
        P4 p4;
        P5 p5;
        int idx = 2;
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

template<class T> class scriptBindObject
{
public:
    static const char* objectTypeName;
    static const char* objectBaseTypeName;
    typedef P<PObject>* PT;

    static int gc_collect(lua_State* L)
    {
        if (lua_istable(L, -1))//When a subclass is destroyed, it's metatable might call the __gc function on it's sub-metatable. So we can get tables here ignore them.
            return 0;
        PT* p = static_cast< PT* >(lua_touserdata(L, -1));
        //printf("Collect: %p %s\n", dynamic_cast<T*>(***p), objectTypeName);
        if (*p)
            delete *p;
        return 0;
    }
    
    static int create(lua_State* L)
    {
        PT* p = static_cast< PT* >(lua_newuserdata(L, sizeof(PT)));
        *p = new P<PObject>();
        T* ptr = new T();
        (**p) = ptr;
        //printf("Create: %p %s\n", ptr, objectTypeName);
        luaL_getmetatable(L, objectTypeName);
        lua_setmetatable(L, -2);
        return 1;
    }
    
    static int isValid(lua_State* L)
    {
        PT* p = static_cast< PT* >(lua_touserdata(L, -1));
        lua_pushboolean(L, (***p) != NULL);
        return 1;
    }
    
    static int destroy(lua_State* L)
    {
        PT* p = static_cast< PT* >(lua_touserdata(L, -1));
        if ((***p) != NULL);
            (***p)->destroy();
        return 0;
    }
    
    static void registerObjectCreation(lua_State* L)
    {
        lua_pushcfunction(L, create);
        lua_setglobal(L, objectTypeName);
        
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
    registerObjectFunctionListItem registerObjectFunctionListItem ## T (scriptBindObject<T>::registerObjectCreation); \
    template <> void scriptBindObject<T>::registerFunctions(lua_State* L, int table)
#define REGISTER_SCRIPT_SUBCLASS(T, BASE) \
    template <> const char* scriptBindObject<T>::objectTypeName = # T; \
    template <> const char* scriptBindObject<T>::objectBaseTypeName = # BASE; \
    registerObjectFunctionListItem registerObjectFunctionListItem ## T (scriptBindObject<T>::registerObjectCreation); \
    template <> void scriptBindObject<T>::registerFunctions(lua_State* L, int table)
#define REGISTER_SCRIPT_CLASS_FUNCTION(T, F) \
    addFunction<T> (L, table, # F , &T::F)
#define REGISTER_SCRIPT_CLASS_CALLBACK(T, C) \
    addCallback<T> (L, table, # C , &T::C)

#endif//SCRIPT_INTERFACE_MAGIC_H

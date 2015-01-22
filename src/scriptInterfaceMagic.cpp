#include "scriptInterfaceMagic.h"

ScriptClassInfo* scriptClassInfoList;

static string getObjectClassNameRecursive(P<PObject> object, ScriptClassInfo* info)
{
    if (info->check_function && info->check_function(object))
    {
        for(unsigned int n=0; n<info->child_classes.size(); n++)
        {
            string sub_name = getObjectClassNameRecursive(object, info->child_classes[n]);
            if (sub_name != "")
                return sub_name;
        }
        return info->class_name;
    }
    return "";
}

string getScriptClassClassNameFromObject(P<PObject> object)
{
    static bool child_relations_set = false;
    if (!child_relations_set)
    {
        for(ScriptClassInfo* item = scriptClassInfoList; item != NULL; item = item->next)
        {
            if (item->base_class_name != "")
            {
                for(ScriptClassInfo* item_parent = scriptClassInfoList; item_parent != NULL; item_parent = item_parent->next)
                {
                    if (item->base_class_name == item_parent->class_name)
                    {
                        item_parent->child_classes.push_back(item);
                    }
                }
            }
        }
        child_relations_set = true;
    }
    
    for(ScriptClassInfo* item = scriptClassInfoList; item != NULL; item = item->next)
    {
        if (item->base_class_name == "")
        {
            string ret = getObjectClassNameRecursive(object, item);
            if (ret != "")
                return ret;
        }
    }
    return "";
}

template<> int convert<bool>::returnType(lua_State* L, bool b)
{
    lua_pushboolean(L, b);
    return 1;
}

template<> int convert<string>::returnType(lua_State* L, string s)
{
    lua_pushstring(L, s.c_str());
    return 1;
}

template<> void convert<const char*>::param(lua_State* L, int& idx, const char*& str)
{
    str = luaL_checkstring(L, idx++);
}

template<> void convert<string>::param(lua_State* L, int& idx, string& str)
{
    str = luaL_checkstring(L, idx++);
}

template<> void convert<bool>::param(lua_State* L, int& idx, bool& b)
{
    b = lua_toboolean(L, idx++);
}

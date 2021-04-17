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

string getScriptClassClassNameFromObject(const P<PObject>& object)
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

template<> int convert<string>::returnType(lua_State* L, const string& s)
{
    lua_pushlstring(L, s.c_str(), s.length());
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

template<> void convert<sf::Color>::param(lua_State* L, int& idx, sf::Color& color)
{
    color = sf::Color::White;
    string str = string(luaL_checkstring(L, idx++)).lower();
    if (str == "black") { color = sf::Color::Black; return; }
    else if (str == "white") { color = sf::Color::White; return; }
    else if (str == "red") { color = sf::Color::Red; return; }
    else if (str == "green") { color = sf::Color::Green; return; }
    else if (str == "blue") { color = sf::Color::Blue; return; }
    else if (str == "yellow") { color = sf::Color::Yellow; return; }
    else if (str == "magenta") { color = sf::Color::Magenta; return; }
    else if (str == "cyan") { color = sf::Color::Cyan; return; }
    
    if (str.startswith("#") && str.length() == 7)
    {
        color.r = static_cast<uint8_t>(str.substr(1, 2).toInt(16));
        color.g = static_cast<uint8_t>(str.substr(3, 2).toInt(16));
        color.b = static_cast<uint8_t>(str.substr(5, 2).toInt(16));
    }
    
    std::vector<string> parts = str.split(",");
    if (parts.size() == 3)
    {
        color.r = static_cast<uint8_t>(parts[0].toInt());
        color.g = static_cast<uint8_t>(parts[1].toInt());
        color.b = static_cast<uint8_t>(parts[2].toInt());
    }
}

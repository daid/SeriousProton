#ifndef _SCRIPTINTERFACEMAGIC_HPP_
#define _SCRIPTINTERFACEMAGIC_HPP_

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
        color.r = str.substr(1, 2).toInt(16);
        color.g = str.substr(3, 2).toInt(16);
        color.b = str.substr(5, 2).toInt(16);
    }
    
    std::vector<string> parts = str.split(",");
    if (parts.size() == 3)
    {
        color.r = parts[0].toInt();
        color.g = parts[1].toInt();
        color.b = parts[2].toInt();
    }
}
#endif /* _SCRIPTINTERFACEMAGIC_HPP_ */

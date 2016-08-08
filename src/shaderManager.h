#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H

#include <SFML/Graphics.hpp>
#include "stringImproved.h"

class ShaderInfo
{
public:
    sf::Shader* shader;
};

class ShaderManager
{
public:
    static sf::Shader* getShader(string name);

private:
    static std::map<string, ShaderInfo> shaders;
};

#endif//SHADER_MANAGER_H

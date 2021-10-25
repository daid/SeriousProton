#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H

#include "graphics/shader.h"
#include "stringImproved.h"

class ShaderManager
{
public:
    static sp::Shader* getShader(string name);

private:
    static std::map<string, sp::Shader*> shaders;
};

#endif//SHADER_MANAGER_H

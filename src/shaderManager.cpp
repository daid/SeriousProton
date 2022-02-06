#include "shaderManager.h"
#include "resources.h"
#include "logging.h"

std::map<string, sp::Shader*> ShaderManager::shaders;

sp::Shader* ShaderManager::getShader(string name)
{
    auto it = shaders.find(name);
    if (it == shaders.end())
    {
        LOG(INFO) << "Loading shader: " << name;
        auto defines = name.split(":");
        auto filename = defines[0];
        defines.erase(defines.begin());
        P<ResourceStream> code_stream = getResourceStream(filename + ".shader");
        if (!code_stream)
        {
            shaders[name] = nullptr;
            return nullptr;
        }
        sp::Shader* shader = new sp::Shader(name, code_stream, defines);
        shaders[name] = shader;
        return shader;
    }
    return it->second;
}

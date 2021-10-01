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
        P<ResourceStream> vertexStream = getResourceStream(name + ".vert");
        P<ResourceStream> fragmentStream = getResourceStream(name + ".frag");
        sp::Shader* shader = new sp::Shader(name, vertexStream, fragmentStream);
        shaders[name] = shader;
        return shader;
    }
    return it->second;
}

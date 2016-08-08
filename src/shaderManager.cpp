#include "shaderManager.h"
#include "resources.h"
#include "logging.h"

std::map<string, ShaderInfo> ShaderManager::shaders;

sf::Shader* ShaderManager::getShader(string name)
{
    if (!sf::Shader::isAvailable())
        return nullptr;
    
    auto it = shaders.find(name);
    if (it == shaders.end())
    {
        sf::Shader* shader = new sf::Shader();
        shaders[name].shader = shader;
        P<ResourceStream> vertexStream = getResourceStream(name + ".vert");
        P<ResourceStream> fragmentStream = getResourceStream(name + ".frag");
        LOG(INFO) << "Loading shader: " << name;
        shader->loadFromStream(**vertexStream, **fragmentStream);
        return shader;
    }
    return it->second.shader;
}

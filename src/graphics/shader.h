#ifndef SP_GRAPHICS_SHADER_H
#define SP_GRAPHICS_SHADER_H

#include "nonCopyable.h"
#include "resources.h"
#include <unordered_map>


namespace sp {

class Shader : sp::NonCopyable
{
public:
    Shader(string name, string vertex_code, string fragment_code);
    Shader(string name, P<ResourceStream> vertexStream, P<ResourceStream> fragmentStream);

    void bind();
    int getUniformLocation(const string& name);
    int getAttributeLocation(const string& name);
private:
    std::unordered_map<string, int> attribute_mapping;
    std::unordered_map<string, int> uniform_mapping;

    string name;
    string vertex_code;
    string fragment_code;
    unsigned int program;

    bool compileShader();
};

}

#endif//SP_GRAPHICS_SHADER_H

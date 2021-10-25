#ifndef SP_GRAPHICS_SHADER_H
#define SP_GRAPHICS_SHADER_H

#include "nonCopyable.h"
#include "resources.h"
#include <unordered_map>


namespace sp {

class Shader : sp::NonCopyable
{
public:
    Shader(const string& name, const string& code, const std::vector<string>& defines = {}, const std::unordered_map<string, int>& attribute_mapping = {});
    Shader(const string& name, P<ResourceStream> code_stream, const std::vector<string>& defines = {}, const std::unordered_map<string, int>& attribute_mapping = {});

    Shader(const string& name, const string& code, const std::unordered_map<string, int>& attribute_mapping);
    Shader(const string& name, P<ResourceStream> code_stream, const std::unordered_map<string, int>& attribute_mapping);

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

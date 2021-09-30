#include "graphics/shader.h"
#include "graphics/opengl.h"
#include <limits>

namespace sp {

Shader::Shader(string vertex_code, string fragment_code)
: vertex_code(vertex_code), fragment_code(fragment_code)
{
    program = std::numeric_limits<unsigned int>::max();
}

Shader::Shader(P<ResourceStream> vertexStream, P<ResourceStream> fragmentStream)
{
    if (!vertexStream || !fragmentStream)
        return;
    vertex_code.resize(vertexStream->getSize());
    fragment_code.resize(fragmentStream->getSize());
    program = std::numeric_limits<unsigned int>::max();
}

static unsigned int compileShader(const char* code, int type)
{
    int success;
    unsigned int shader_handle = glCreateShader(type);
    glShaderSource(shader_handle, 1, &code, nullptr);
    glCompileShader(shader_handle);
    glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char log[1024];
        glGetShaderInfoLog(shader_handle, sizeof(log), nullptr, log);
        LOG(Error, "Compile error in shader:", log);
        glDeleteShader(shader_handle);
#ifdef ANDROID
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Shader error", log, nullptr);
#endif
        return 0;
    }
    return shader_handle;
}

bool Shader::compileShader()
{
    program = 0;
    unsigned int vertex_shader_handle = sp::compileShader(vertex_code.c_str(), GL_VERTEX_SHADER);
    if (!vertex_shader_handle)
        return false;
    unsigned int fragment_shader_handle = sp::compileShader(fragment_code.c_str(), GL_FRAGMENT_SHADER);
    if (!fragment_shader_handle)
    {
        glDeleteShader(vertex_shader_handle);
        return false;
    }
    program = glCreateProgram();
    glAttachShader(program, vertex_shader_handle);
    glAttachShader(program, fragment_shader_handle);
    glLinkProgram(program);
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        LOG(Error, "Link error in shader:", log);
        glDeleteProgram(program);
        glDeleteShader(vertex_shader_handle);
        glDeleteShader(fragment_shader_handle);
        program = 0;
#ifdef ANDROID
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Shader error", log, nullptr);
#endif
        return false;
    }
    glDetachShader(program, vertex_shader_handle);
    glDetachShader(program, fragment_shader_handle);
    return true;
}

void Shader::bind()
{
    if (program == std::numeric_limits<unsigned int>::max())
    {
        compileShader();
    }
    if (!program)
        return;
    glUseProgram(program);
}

int Shader::getUniformLocation(const string& name)
{
    auto it = uniform_mapping.find(name);
    if (it != uniform_mapping.end())
        return it->second;
        
    int location = glGetUniformLocation(program, name.c_str());
    if (location == -1)
        LOG(Debug, "Failed to find uniform:", name);
    uniform_mapping[name] = location;
    return location;
}

int Shader::getAttributeLocation(const string& name)
{
    auto it = attribute_mapping.find(name);
    if (it != attribute_mapping.end())
        return it->second;
        
    int location = glGetAttribLocation(program, name.c_str());
    if (location == -1)
        LOG(Debug, "Failed to find uniform:", name);
    attribute_mapping[name] = location;
    return location;
}

}
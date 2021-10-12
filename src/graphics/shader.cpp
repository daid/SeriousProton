#include "graphics/shader.h"
#include "graphics/opengl.h"
#include <limits>
#include <SDL.h>

namespace sp {

static Shader* current_shader = nullptr;

#if defined(ANDROID)
static const char* shader_header = "#version 100\nprecision mediump float;\n";
#else
static const char* shader_header = "#version 120\n";
#endif


Shader::Shader(string name, string code, const std::vector<string>& defines)
: name(name), vertex_code(shader_header), fragment_code(shader_header)
{
    for(auto str : defines)
    {
        vertex_code += "#define " + string(str) + " 1\n";
        fragment_code += "#define " + string(str) + " 1\n";
    }

    program = std::numeric_limits<unsigned int>::max();
    int mode = -1;
    int line_nr = 1;
    for(auto line : code.split("\n"))
    {
        if (line.strip().lower() == "[vertex]") {
            mode = 0;
            vertex_code += "#line " + string(line_nr) + "\n";
        } else if (line.strip().lower() == "[fragment]") {
            mode = 1;
            fragment_code += "#line " + string(line_nr) + "\n";
        } else if (mode == 0) {
            vertex_code += line + "\n";
        } else if (mode == 1) {
            fragment_code += line + "\n";
        }
        line_nr++;
    }
}

Shader::Shader(string name, P<ResourceStream> code_stream, const std::vector<string>& defines)
: Shader(name, code_stream->readAll(), defines)
{
}

Shader::Shader(string name, P<ResourceStream> vertexStream, P<ResourceStream> fragmentStream)
: name(name)
{
    if (!vertexStream || !fragmentStream)
    {
        LOG(Error, "Missing input streams for shader:", name);
        return;
    }
    vertex_code.resize(vertexStream->getSize());
    vertexStream->read(vertex_code.data(), vertex_code.size());
    fragment_code.resize(fragmentStream->getSize());
    fragmentStream->read(fragment_code.data(), fragment_code.size());
    program = std::numeric_limits<unsigned int>::max();
}

static unsigned int compileShader(const string& name, const char* code, int type)
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
        LOG(Error, "Compile error in shader:", name, ":", log);
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
    unsigned int vertex_shader_handle = sp::compileShader(name, vertex_code.c_str(), GL_VERTEX_SHADER);
    if (!vertex_shader_handle)
        return false;
    unsigned int fragment_shader_handle = sp::compileShader(name, fragment_code.c_str(), GL_FRAGMENT_SHADER);
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
        LOG(Error, "Link error in shader:", name, log);
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
    current_shader = this;
    if (!program)
        return;
    glUseProgram(program);
}

int Shader::getUniformLocation(const string& name)
{
    assert(current_shader == this);
    auto it = uniform_mapping.find(name);
    if (it != uniform_mapping.end())
        return it->second;
        
    int location = glGetUniformLocation(program, name.c_str());
    if (location == -1)
        LOG(Debug, "Failed to find uniform:", name, " in ", this->name);
    uniform_mapping[name] = location;
    return location;
}

int Shader::getAttributeLocation(const string& name)
{
    assert(current_shader == this);
    auto it = attribute_mapping.find(name);
    if (it != attribute_mapping.end())
        return it->second;
        
    int location = glGetAttribLocation(program, name.c_str());
    if (location == -1)
        LOG(Debug, "Failed to find attribute:", name, " in ", this->name);
    attribute_mapping[name] = location;
    return location;
}

}

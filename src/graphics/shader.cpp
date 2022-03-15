#include "graphics/shader.h"
#include "graphics/opengl.h"
#include <limits>
#include <SDL.h>

namespace sp {

static Shader* current_shader = nullptr;

static const char* vertex_shader_header_es = "#version 100\nprecision highp float;\n";
static const char* fragment_shader_header_es = "#version 100\nprecision mediump float;\n";
static const char* vertex_shader_header = "#version 120\n";
static const char* fragment_shader_header = "#version 120\n";

Shader::Shader(const string& name, const string& code, const std::vector<string>& defines, const std::unordered_map<string, int>& attribute_mapping)
: attribute_mapping{attribute_mapping}, name(name)
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

Shader::Shader(const string& name, P<ResourceStream> code_stream, const std::vector<string>& defines, const std::unordered_map<string, int>& attribute_mapping)
: Shader(name, code_stream->readAll(), defines, attribute_mapping)
{
}

Shader::Shader(const string& name, const string& code, const std::unordered_map<string, int>& attribute_mapping)
    :Shader(name, code, {}, attribute_mapping)
{}

Shader::Shader(const string& name, P<ResourceStream> code_stream, const std::unordered_map<string, int>& attribute_mapping)
    :Shader(name, code_stream, {}, attribute_mapping)
{}

static unsigned int compileShader(const string& name, const char* header, const char* code, int type)
{
    int success;
    unsigned int shader_handle = glCreateShader(type);
    const char* code_ptr[2] = {header, code};
    glShaderSource(shader_handle, 2, code_ptr, nullptr);
    glCompileShader(shader_handle);
    glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char log[1024];
        glGetShaderInfoLog(shader_handle, static_cast<GLsizei>(sizeof(log)), nullptr, log);
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

    unsigned int vertex_shader_handle = sp::compileShader(name, gl::contextIsES ? vertex_shader_header_es : vertex_shader_header, vertex_code.c_str(), GL_VERTEX_SHADER);
    if (!vertex_shader_handle)
        return false;
    unsigned int fragment_shader_handle = sp::compileShader(name, gl::contextIsES ? fragment_shader_header_es : fragment_shader_header, fragment_code.c_str(), GL_FRAGMENT_SHADER);
    if (!fragment_shader_handle)
    {
        glDeleteShader(vertex_shader_handle);
        return false;
    }
    program = glCreateProgram();

    // Remap attribute locations if they're already set (has to be done early)
    for (const auto& [name, position] : attribute_mapping)
    {
        glBindAttribLocation(program, position, name.data());
    }

    glAttachShader(program, vertex_shader_handle);
    glAttachShader(program, fragment_shader_handle);
    glLinkProgram(program);
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[1024];
        glGetProgramInfoLog(program, static_cast<GLsizei>(sizeof(log)), nullptr, log);
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
    SDL_assert(current_shader == this);
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
    SDL_assert(current_shader == this);
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

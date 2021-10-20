#include "graphics/opengl.h"

#include <logging.h>
#include <SDL_log.h>
#include <SDL_video.h>
#include <SDL_assert.h>

#include "stringImproved.h"

extern "C" {
    int SP_texture_compression_etc2 = 0;
}

namespace {

    const char* debugTypeLabel(GLenum type)
    {
        switch (type)
        {
        case GL_DEBUG_TYPE_ERROR:
            return "[Error] ";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            return "[Deprecated] ";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            return "[Undefined] ";
        case GL_DEBUG_TYPE_PORTABILITY:
            return "[Portability] ";
        case GL_DEBUG_TYPE_PERFORMANCE:
            return "[Performance] ";
        case GL_DEBUG_TYPE_OTHER:
            return "[Other] ";
        case GL_DEBUG_TYPE_MARKER:
            return "[Marker] ";
        default:
            return "[Unknown] ";
        }
    }

    const char* sourceLabel(GLenum source)
    {
        switch (source)
        {
            case GL_DEBUG_SOURCE_API:
                return "[API] ";
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:  
                return "[WindowSystem] ";
            case GL_DEBUG_SOURCE_SHADER_COMPILER:
                return "[ShaderCompiler] ";
            case GL_DEBUG_SOURCE_THIRD_PARTY:
                return "[ThirdParty] ";
            case GL_DEBUG_SOURCE_APPLICATION:
                return "[Application] ";
            case GL_DEBUG_SOURCE_OTHER:
                return "[Other] ";
            default:
                SDL_assert(false);
                return "[Unknown] ";
        }
    }
    ELogLevel severityCast(GLenum severity)
    {
        switch (severity)
        {
        case GL_DEBUG_SEVERITY_HIGH:
            return LOGLEVEL_ERROR;
        case GL_DEBUG_SEVERITY_MEDIUM:
            return LOGLEVEL_WARNING;
        case GL_DEBUG_SEVERITY_LOW:
            return LOGLEVEL_INFO;
        default:
            SDL_assert(false);
            [[fallthrough]];
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            return LOGLEVEL_DEBUG;
        }
    }

    void APIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* /*user*/)
    {
        Logging entry(severityCast(severity), __FILE__, __LINE__, "", "[GL] ", sourceLabel(source), debugTypeLabel(type), string(id) + " ", message);
    }
}


namespace sp {

void initOpenGL()
{
    static bool init_done = false;
    if (init_done)
        return;
    init_done = true;

    int profile_mask = 0;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profile_mask);
    
    if (profile_mask == SDL_GL_CONTEXT_PROFILE_ES)
    {
        if (!gladLoadGLES2Loader(&SDL_GL_GetProcAddress))
            exit(1);
    }
    else
    {
        if (!gladLoadGLLoader(&SDL_GL_GetProcAddress))
            exit(1);
    }

    // Find out supported compressed textures.
    GLint count{};
    glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &count);
    std::vector<GLint> formats(count);
    glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, formats.data());
    SP_texture_compression_etc2 = std::find(std::begin(formats), std::end(formats), GL_COMPRESSED_RGBA8_ETC2_EAC) != std::end(formats);

    SDL_assert_always(glGetError() == GL_NO_ERROR);
}

namespace gl {
    bool enableDebugOutput(bool synchronous)
    {
        if (GLAD_GL_KHR_debug)
        {
            glDebugMessageCallback(debugCallback, nullptr);
            glDebugMessageControl(
                GL_DONT_CARE /* any source */,
                GL_DONT_CARE /* any type */,
                GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
            glEnable(synchronous ? GL_DEBUG_OUTPUT_SYNCHRONOUS : GL_DEBUG_OUTPUT);
            return true;
        }

        return false;
    }
}

#ifdef SP_ENABLE_OPENGL_TRACING
    namespace details {
        //#define FULL_LOG
        void traceOpenGLCall(const char* function_name, const char* source_file, const char* source_function, int source_line_number, const string& parameters)
        {
            int error = glad_glGetError();
    #ifdef FULL_LOG
    #if defined(ANDROID) || defined(__EMSCRIPTEN__)
            LOG(Debug, "GL_TRACE", source_file, source_line_number, source_function, function_name, parameters);
            if (error)
                LOG(Error, "GL_TRACE ERROR", error);
    #else
            static FILE* f = nullptr;
            if (!f)
                f = fopen("opengl.trace.txt", "wt");
            fprintf(f, "%80s:%4d %60s %s %s\n", source_file, source_line_number, source_function, function_name, parameters.c_str());
            if (error)
                fprintf(f, "ERROR: %d\n", error);
    #endif
    #else
            SDL_assert_always(error == GL_NO_ERROR);
            if (error != GL_NO_ERROR)
            {
                LOG(Error, "glGetError:", error, "@", source_file, ":", source_line_number, ":", source_function, ":", function_name, ":", parameters);
            }

    #endif
        }
    }
#endif//SP_ENABLE_OPENGL_TRACING
}//namespace sp
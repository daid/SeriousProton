#include "graphics/opengl.h"

#include <logging.h>
#include <SDL_log.h>
#include <SDL_video.h>
#include <SDL_assert.h>

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
    SDL_assert_always(glGetError() == GL_NO_ERROR);
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
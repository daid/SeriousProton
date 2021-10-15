#include "logging.h"

#include <SDL_log.h>

#ifdef WIN32
#include <cstdio>
#else
#include <SDL_rwops.h>
#endif

namespace
{
    void sdlCallback(void* userdata, int /*category*/, SDL_LogPriority priority, const char* message)
    {
#ifdef WIN32
        auto stream = static_cast<FILE*>(userdata);
        auto write = [stream](const void* buffer, size_t size, size_t num)
        {
            return fwrite(buffer, size, num, stream);
        };
#else
        auto stream = static_cast<SDL_RWops*>(userdata);
        auto write = [stream](const void* buffer, size_t size, size_t num)
        {
            return SDL_RWwrite(stream, buffer, size, num);
        };
#endif
        switch (priority)
        {
        case SDL_LOG_PRIORITY_VERBOSE:
            write("[VERBOSE]: ", 12, 1);
            break;

        case SDL_LOG_PRIORITY_DEBUG:
            write("[DEBUG]: ", 9, 1);
            break;

        case SDL_LOG_PRIORITY_INFO:
            write("[INFO]: ", 8, 1);
            break;

        case SDL_LOG_PRIORITY_WARN:
            write("[WARN]: ", 8, 1);
            break;

        case SDL_LOG_PRIORITY_ERROR:
            write("[ERROR]: ", 9, 1);
            break;

        case SDL_LOG_PRIORITY_CRITICAL:
            write("[CRITICAL]: ", 12, 1);
            break;
        }
        write(message, SDL_strlen(message), 1);
        write("\n", 1, 1);
#ifdef WIN32
        fflush(stream);
#endif
    }

    constexpr SDL_LogPriority asSDLPriority(ELogLevel level)
    {
        auto priority = SDL_LOG_PRIORITY_VERBOSE;
        switch (level)
        {
        case LOGLEVEL_DEBUG:
            priority = SDL_LOG_PRIORITY_DEBUG;
            break;
        case LOGLEVEL_INFO:
            priority = SDL_LOG_PRIORITY_INFO;
            break;
        case LOGLEVEL_WARNING:
            priority = SDL_LOG_PRIORITY_WARN;
            break;
        case LOGLEVEL_ERROR:
            priority = SDL_LOG_PRIORITY_ERROR;
            break;
        }

        return priority;
    }
}

ELogLevel Logging::global_level = LOGLEVEL_ERROR;

#define print_func(str) stream << str

Logging::Logging(ELogLevel in_level, std::string_view file, int /*line*/, std::string_view function_name)
    :do_logging{in_level >= global_level}, level{in_level}
{
}

Logging::~Logging()
{
    if (do_logging)
    {
        auto sdl_level = SDL_LOG_PRIORITY_VERBOSE;
        switch (level)
        {
        case LOGLEVEL_DEBUG:
            sdl_level = SDL_LOG_PRIORITY_DEBUG;
            break;
        case LOGLEVEL_INFO:
            sdl_level = SDL_LOG_PRIORITY_INFO;
            break;
        case LOGLEVEL_WARNING:
            sdl_level = SDL_LOG_PRIORITY_WARN;
            break;
        case LOGLEVEL_ERROR:
            sdl_level = SDL_LOG_PRIORITY_ERROR;
            break;
        }

        SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, sdl_level, "%s", stream.str().c_str());
    }
}

const Logging& operator<<(const Logging& log, const char* str)
{
    if (log.do_logging)
        log.stream << str;
    return log;
}

void Logging::setLogLevel(ELogLevel level)
{
    global_level = level;
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, asSDLPriority(global_level));
}

void Logging::setLogFile(std::string_view filename)
{
    // Windows/SDL quirk (2.0.14):
    //  On Win32 write access is exclusive with SDL's implementation:
    //  https://github.com/libsdl-org/SDL/blob/4cd981609b50ed273d80c635c1ca4c1e5518fb21/src/file/SDL_rwops.c#L122
    //  And by default (ie - public binaries) the STDIO interface is compiled out on win32.
    //  Since this prevents someone to `tail` the log while the game is running,
    //  fallback on cstdio on windows.

    SDL_LogOutputFunction current = nullptr;
    void* current_data = nullptr;
    SDL_LogGetOutputFunction(&current, &current_data);
    if (current == &sdlCallback)
    {
#ifdef WIN32
        auto stream = static_cast<FILE*>(current_data);
        fclose(stream);
#else
        auto stream = static_cast<SDL_RWops*>(current_data);
        SDL_RWclose(stream);
#endif
    }

#ifdef WIN32
    auto handle = fopen(filename.data(), "wt");
#else
    auto handle = SDL_RWFromFile(filename.data(), "wt");
#endif

    SDL_LogSetOutputFunction(&sdlCallback, handle);
}

#include "logging.h"

#include <SDL_log.h>
#include <array>
#include <string>

#ifdef WIN32
// Windows/SDL quirk (2.0.14):
//  On Win32 write access is exclusive with SDL's implementation:
//  https://github.com/libsdl-org/SDL/blob/4cd981609b50ed273d80c635c1ca4c1e5518fb21/src/file/SDL_rwops.c#L122
//  And by default (ie - public binaries) the STDIO interface is compiled out on win32.
//  Since this prevents someone to `tail` the log while the game is running,
//  fallback on cstdio on windows.
#define SP_LOGGING_FALLBACK_STDIO 1
#else
#define SP_LOGGING_FALLBACK_STDIO 0
#endif

#include <cstdio>
#include <SDL_rwops.h>


namespace
{
    const std::array<std::string, SDL_NUM_LOG_PRIORITIES> priority_labels{
        "[UNKNOWN ]: ",
        "[VERBOSE ]: ",
        "[DEBUG   ]: ",
        "[INFO    ]: ",
        "[WARNING ]: ",
        "[ERROR   ]: ",
        "[CRITICAL]: "
    };

    void stdioCallback(void* userdata, int /*category*/, SDL_LogPriority priority, const char* message)
    {
        auto stream = static_cast<FILE*>(userdata);
        auto write = [stream](const void* buffer, size_t size, size_t num)
        {
            return fwrite(buffer, size, num, stream);
        };
        const auto& label = priority_labels[priority];
        write(label.data(), label.size(), 1);
        write(message, SDL_strlen(message), 1);
        write("\n", 1, 1);
        fflush(stream);
    }

    void sdlCallback(void* userdata, int /*category*/, SDL_LogPriority priority, const char* message)
    {
        auto stream = static_cast<SDL_RWops*>(userdata);
        auto write = [stream](const void* buffer, size_t size, size_t num)
        {
            return SDL_RWwrite(stream, buffer, size, num);
        };
        const auto& label = priority_labels[priority];
        write(label.data(), label.size(), 1);
        write(message, SDL_strlen(message), 1);
        write("\n", 1, 1);
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
    closeCurrentLogStream();

#if SP_LOGGING_FALLBACK_STDIO
    auto handle = fopen(filename.data(), "wt");
    SDL_LogSetOutputFunction(&stdioCallback, handle);
#else
    auto handle = SDL_RWFromFile(filename.data(), "wt");
    SDL_LogSetOutputFunction(&sdlCallback, handle);
#endif
}

void Logging::setLogStdout()
{
    closeCurrentLogStream();
    SDL_LogSetOutputFunction(&stdioCallback, stdout);
}

void Logging::closeCurrentLogStream()
{
    SDL_LogOutputFunction current = nullptr;
    void* current_data = nullptr;
    SDL_LogGetOutputFunction(&current, &current_data);
    if (current == &stdioCallback) {
        auto stream = static_cast<FILE*>(current_data);
        if (stream != stdout)
            fclose(stream);
    }
    if (current == &sdlCallback) {
        auto stream = static_cast<SDL_RWops*>(current_data);
        SDL_RWclose(stream);
    }
}
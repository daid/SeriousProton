#include "logging.h"

#include <SDL_log.h>
#include <SDL_rwops.h>

namespace
{
    void sdlCallback(void* userdata, int /*category*/, SDL_LogPriority priority, const char* message)
    {
        auto data = static_cast<SDL_RWops*>(userdata);
        switch (priority)
        {
        case SDL_LOG_PRIORITY_VERBOSE:
            SDL_RWwrite(data, "[VERBOSE]: ", 12, 1);
            break;

        case SDL_LOG_PRIORITY_DEBUG:
            SDL_RWwrite(data, "[DEBUG]: ", 9, 1);
            break;

        case SDL_LOG_PRIORITY_INFO:
            SDL_RWwrite(data, "[INFO]: ", 8, 1);
            break;

        case SDL_LOG_PRIORITY_WARN:
            SDL_RWwrite(data, "[WARN]: ", 8, 1);
            break;

        case SDL_LOG_PRIORITY_ERROR:
            SDL_RWwrite(data, "[ERROR]: ", 9, 1);
            break;

        case SDL_LOG_PRIORITY_CRITICAL:
            SDL_RWwrite(data, "[CRITICAL]: ", 12, 1);
            break;
        }
        SDL_RWwrite(data, message, SDL_strlen(message), 1);
        SDL_RWwrite(data, "\n", 1, 1);
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
    :level{in_level}, do_logging{in_level >= global_level}
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
    SDL_LogOutputFunction current = nullptr;
    void* current_data = nullptr;
    SDL_LogGetOutputFunction(&current, &current_data);
    if (current == &sdlCallback)
    {
        auto stream = static_cast<SDL_RWops*>(current_data);
        SDL_RWclose(stream);
        SDL_FreeRW(stream);
    }

    SDL_LogSetOutputFunction(&sdlCallback, SDL_RWFromFile(filename.data(), "wt"));
}

#include <stdio.h>
#ifdef __ANDROID__
#include <android/log.h>
#endif//__ANDROID__

#include "logging.h"

ELogLevel Logging::global_level = LOGLEVEL_ERROR;
FILE* Logging::log_stream = nullptr;

#ifdef __ANDROID__
#define print_func(str) __android_log_write(ANDROID_LOG_INFO, "SeriousProton", str)
#else
#define print_func(str) fputs(str, Logging::log_stream)
#endif

Logging::Logging(ELogLevel level, std::string_view file, int /*line*/, std::string_view function_name)
{
    do_logging = level >= global_level;
    
    if (do_logging)
    {
        if (log_stream == nullptr)
            log_stream = stderr;
        switch(level)
        {
        case LOGLEVEL_DEBUG:
            print_func("[DEBUG] ");
            break;
        case LOGLEVEL_INFO:
            print_func("[INFO]  ");
            break;
        case LOGLEVEL_WARNING:
            print_func("[WARN]  ");
            break;
        case LOGLEVEL_ERROR:
            print_func("[ERROR] ");
            break;
        }
    }
}

Logging::~Logging()
{
    if (do_logging)
        print_func("\n");
}

const Logging& operator<<(const Logging& log, const char* str)
{
    if (log.do_logging)
        print_func(str);
    return log;
}

void Logging::setLogLevel(ELogLevel level)
{
    global_level = level;
}

void Logging::setLogFile(std::string_view filename)
{
    log_stream = fopen(filename.data(), "wt");
    #ifdef _WIN32
    // win32 doesn't support line buffering. #1042
    // Instead, don't buffer logging at all on Windows.
    setvbuf(log_stream, NULL, _IONBF, 0);
    #endif//_WIN32
}

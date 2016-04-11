#include <stdio.h>
#ifdef __ANDROID__
#include <android/log.h>
#endif//__ANDROID__

#include "logging.h"

ELogLevel Logging::global_level = LOGLEVEL_ERROR;
FILE* Logging::log_stream = nullptr;

#ifdef __ANDROID__
#define print_func(...) __android_log_print(ANDROID_LOG_INFO, "SeriousProton", __VA_ARGS__)
#else
#define print_func(...) fprintf(Logging::log_stream, __VA_ARGS__)
#endif

Logging::Logging(ELogLevel level, string file, int line, string function_name)
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
        print_func("%s", str);
    return log;
}

void Logging::setLogLevel(ELogLevel level)
{
    global_level = level;
}

void Logging::setLogFile(string filename)
{
    log_stream = fopen(filename.c_str(), "wt");
}

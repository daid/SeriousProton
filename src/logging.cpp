#include <stdio.h>

#include "logging.h"

ELogLevel Logging::global_level = LOGLEVEL_ERROR;

Logging::Logging(ELogLevel level, string file, int line, string function_name)
{
    do_logging = level >= global_level;
    
    if (do_logging)
    {
        switch(level)
        {
        case LOGLEVEL_DEBUG:
            printf("[DEBUG] ");
            break;
        case LOGLEVEL_INFO:
            printf("[INFO]  ");
            break;
        case LOGLEVEL_WARNING:
            printf("[WARN]  ");
            break;
        case LOGLEVEL_ERROR:
            printf("[ERROR] ");
            break;
        }
    }
}

Logging::~Logging()
{
    if (do_logging)
        printf("\n");
}

const Logging& operator<<(const Logging& log, const char* str)
{
    if (log.do_logging)
        printf("%s", str);
    return log;
}

void Logging::setLogLevel(ELogLevel level)
{
    global_level = level;
}

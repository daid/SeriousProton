#include <stdio.h>

#include "logging.h"

Logging::Logging(ELogLevel level, string file, int line, string function_name)
{
    printf(">");
}

Logging::~Logging()
{
    printf("\n");
}

const Logging& operator<<(const Logging& log, const char* str)
{
    printf("%s", str);
    return log;
}

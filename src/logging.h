#ifndef LOGGING_H
#define LOGGING_H
#include <sstream>
#include <string_view>

#include <glm/vec2.hpp>
#include "nonCopyable.h"
#include "stringImproved.h"
#if defined(_MSC_VER)
#define LOG(LEVEL, ...) Logging{LOGLEVEL_ ## LEVEL, __FILE__, __LINE__, __FUNCTION__ , ##__VA_ARGS__}
#else
#define LOG(LEVEL, ...) Logging(LOGLEVEL_ ## LEVEL, __FILE__, __LINE__, __PRETTY_FUNCTION__ , ##__VA_ARGS__)
#endif

enum ELogLevel
{
    LOGLEVEL_DEBUG = 0,
    LOGLEVEL_INFO,
    LOGLEVEL_WARNING,
    LOGLEVEL_ERROR,

    LOGLEVEL_Debug = 0,
    LOGLEVEL_Info,
    LOGLEVEL_Warning,
    LOGLEVEL_Error
};

class Logging : sp::NonCopyable
{
    static ELogLevel global_level;
    bool do_logging;
    mutable std::ostringstream stream;
    ELogLevel level;
public:
    Logging(ELogLevel level, std::string_view file, int line, std::string_view function_name);
    template<typename... ARGS>
    Logging(ELogLevel level, std::string_view file, int line, std::string_view function_name, ARGS&&... args)
    : Logging(level, file, line, function_name)
    {
        ((*this << args), ...);
    }
    ~Logging();
    
    static void setLogLevel(ELogLevel level);
    static void setLogFile(std::string_view filename);
    
    friend const Logging& operator<<(const Logging& log, const char* str);
};

const Logging& operator<<(const Logging& log, const char* str);
inline const Logging& operator<<(const Logging& log, const std::string& s) { return log << (s.c_str()); }
inline const Logging& operator<<(const Logging& log, const int i) { return log << string(i).c_str(); }
inline const Logging& operator<<(const Logging& log, const unsigned int i) { return log << string(i).c_str(); }
inline const Logging& operator<<(const Logging& log, const long i) { return log << string(int(i)).c_str(); }
inline const Logging& operator<<(const Logging& log, const unsigned long i) { return log << string(int(i)).c_str(); }
inline const Logging& operator<<(const Logging& log, const float f) { return log << string(f).c_str(); }
inline const Logging& operator<<(const Logging& log, const double f) { return log << string(float(f), 2).c_str(); }
inline const Logging& operator<<(const Logging& log, const unsigned long long i) { return log << string(int(i)).c_str(); }
template<typename T, glm::qualifier Q> inline const Logging& operator<<(const Logging& log, const glm::vec<2, T, Q> v) { return log << v.x << "," << v.y; }

#endif//LOGGING_H

#ifndef LOGGING_H
#define LOGGING_H

#include <SFML/System.hpp>
#include <SFML/Graphics/Rect.hpp>
#include "stringImproved.h"

#define LOG(LEVEL) Logging(LOGLEVEL_ ## LEVEL, __FILE__, __LINE__, __PRETTY_FUNCTION__)

enum ELogLevel
{
    LOGLEVEL_DEBUG,
    LOGLEVEL_INFO,
    LOGLEVEL_WARNING,
    LOGLEVEL_ERROR
};

class Logging : sf::NonCopyable
{
    static ELogLevel global_level;
    static FILE* log_stream;
    bool do_logging;
public:
    Logging(ELogLevel level, string file, int line, string function_name);
    ~Logging();
    
    static void setLogLevel(ELogLevel level);
    static void setLogFile(string filename);
    
    friend const Logging& operator<<(const Logging& log, const char* str);
};

const Logging& operator<<(const Logging& log, const char* str);
inline const Logging& operator<<(const Logging& log, const std::string& s) { return log << (s.c_str()); }
inline const Logging& operator<<(const Logging& log, const int i) { return log << string(i).c_str(); }
inline const Logging& operator<<(const Logging& log, const unsigned int i) { return log << string(i).c_str(); }
inline const Logging& operator<<(const Logging& log, const long i) { return log << string(int(i)).c_str(); }
inline const Logging& operator<<(const Logging& log, const unsigned long i) { return log << string(int(i)).c_str(); }
inline const Logging& operator<<(const Logging& log, const float f) { return log << string(f).c_str(); }
inline const Logging& operator<<(const Logging& log, const double f) { return log << string(f, 2).c_str(); }
inline const Logging& operator<<(const Logging& log, const unsigned long long i) { return log << string(int(i)).c_str(); }
template<typename T> inline const Logging& operator<<(const Logging& log, const sf::Vector2<T> v) { return log << v.x << "," << v.y; }
template<typename T> inline const Logging& operator<<(const Logging& log, const sf::Rect<T> v) { return log << v.left << "," << v.top << ":" << v.width << "x" << v.height; }

#endif//LOGGING_H

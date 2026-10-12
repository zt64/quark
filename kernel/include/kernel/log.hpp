#pragma once
#include <cstdarg>

enum LogLevel {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
};

class Logger {
public:
    explicit Logger(const LogLevel initial = LOG_LEVEL_DEBUG) : log_level(initial) {
    }
    void vlog(LogLevel level, const char* fmt, va_list ap) const __attribute__((format(printf, 3, 0)));
    void log(LogLevel level, const char* fmt, ...) const __attribute__((format(printf, 3, 4)));

    __attribute__ ((format (printf, 2, 3)))
    void debug(const char* fmt, ...) const;

    __attribute__ ((format (printf, 2, 3)))
    void info(const char* fmt, ...) const;

    __attribute__ ((format (printf, 2, 3)))
    void warn(const char* fmt, ...) const;

    __attribute__ ((format (printf, 2, 3)))
    void error(const char* fmt, ...) const;
    __attribute__ ((format (printf, 2, 3)))
    void fatal(const char* fmt, ...) const;

private:
    LogLevel log_level;
};

inline Logger logger(LOG_LEVEL_DEBUG);

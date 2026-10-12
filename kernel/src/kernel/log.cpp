#include "kernel/log.hpp"

#include <lib/printf.h>

#include "driver/serial.hpp"
#include "lib/mem.hpp"
#include "lib/string.hpp"

constexpr uint32_t LOG_LINE_MAX = 256;
constexpr uint16_t LOG_MAX_LINES = 512;

static const char* level_names[] = {
    "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

namespace {
    struct LogEntry {
        uint64_t timestamp;
        LogLevel level;
        uint16_t len;
        char payload[LOG_LINE_MAX];
    } __attribute__((packed, aligned(8)));
}

static LogEntry log_buffer[LOG_MAX_LINES];
static uint32_t log_buffer_head = 0;
static uint32_t log_buffer_tail = 0;

void Logger::vlog(const LogLevel level, const char* fmt, va_list ap) const {
    if (level < log_level) return;

    const char* prefix;

    switch (level) {
        case LOG_LEVEL_DEBUG: prefix = "\033[94m"; break;
        case LOG_LEVEL_INFO:  prefix = "\033[92m"; break;
        case LOG_LEVEL_WARN:  prefix = "\033[93m"; break;
        case LOG_LEVEL_ERROR: prefix = "\033[91m"; break;
        case LOG_LEVEL_FATAL: prefix = "\033[95m"; break;
        default:              prefix = ""; break;
    }

    char body[255];
    vsnprintf(body, sizeof(body), fmt, ap);

    char buffer[255];
    snprintf(
        buffer,
        sizeof(buffer),
        "%s[%s] %s\033[0m\n",
        prefix,
        level_names[level],
        body
    );

    const uint32_t next = (log_buffer_head + 1) % LOG_MAX_LINES;

    if (next == log_buffer_tail) {
        return;
    }

    LogEntry& entry = log_buffer[log_buffer_head];

    entry.len = strlen(buffer);
    entry.timestamp = 0;
    entry.level = level;

    memcpy(entry.payload, buffer, entry.len);

    log_buffer_head = next;

    if (serial::available()) {
        printf(entry.payload);
    }

    // Kernel log messages are intentionally serial-only: the screen belongs
    // to the console (and, through it, to whatever user program is
    // running), so we never paint log lines onto the framebuffer here.
}

void Logger::log(const LogLevel level, const char* fmt, ...) const {
    va_list ap;
    va_start(ap, fmt);
    vlog(level, fmt, ap);
    va_end(ap);
}

void Logger::debug(const char* fmt, ...) const {
    va_list ap;
    va_start(ap, fmt);
    vlog(LOG_LEVEL_DEBUG, fmt, ap);
    va_end(ap);
}

void Logger::info(const char* fmt, ...) const {
    va_list ap;
    va_start(ap, fmt);
    vlog(LOG_LEVEL_INFO, fmt, ap);
    va_end(ap);
}

void Logger::warn(const char* fmt, ...) const {
    va_list ap;
    va_start(ap, fmt);
    vlog(LOG_LEVEL_WARN, fmt, ap);
    va_end(ap);
}

void Logger::error(const char* fmt, ...) const {
    va_list ap;
    va_start(ap, fmt);
    vlog(LOG_LEVEL_ERROR, fmt, ap);
    va_end(ap);
}

void Logger::fatal(const char* fmt, ...) const {
    va_list ap;
    va_start(ap, fmt);
    vlog(LOG_LEVEL_FATAL, fmt, ap);
    va_end(ap);
}

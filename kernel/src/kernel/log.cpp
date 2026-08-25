#include "kernel/log.hpp"

#include <lib/printf.h>

#include "driver/fb.hpp"
#include "driver/serial.hpp"
#include "lib/mem.hpp"
#include "lib/string.hpp"

constexpr uint32_t LOG_LINE_MAX = 256;
constexpr uint16_t LOG_MAX_LINES = 512;

static const char* level_names[] = {
    "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

static const uint32_t level_colors[] = {
    0x60A5FA, // DEBUG - blue
    0xA7F3D0, // INFO  - mint
    0xFBBF24, // WARN  - amber
    0xFB7185, // ERROR - rose
    0xC084FC // FATAL - violet
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

void redraw() {
    screen::clear();

    constexpr uint32_t CHAR_WIDTH = 8;
    constexpr float SCALE = 1.5f;
    constexpr auto LINE_HEIGHT = static_cast<uint32_t>(CHAR_WIDTH * SCALE);

    const uint32_t MAX_CHARS = screen::framebuffer.width / static_cast<uint32_t>(CHAR_WIDTH * SCALE);
    const uint32_t MAX_LINES = screen::framebuffer.height / LINE_HEIGHT;

    // First pass: count how many wrapped display lines exist.
    uint32_t total_lines = 0;

    for (uint32_t i = log_buffer_tail; i != log_buffer_head; i = (i + 1) % LOG_MAX_LINES) {
        const LogEntry& entry = log_buffer[i];

        uint32_t offset = 0;
        while (offset < entry.len) {
            uint32_t chars = 0;

            while (offset + chars < entry.len &&
                chars < MAX_CHARS &&
                entry.payload[offset + chars] != '\n') {
                chars++;
            }

            if (chars == MAX_CHARS &&
                offset + chars < entry.len &&
                entry.payload[offset + chars] != '\n') {
                for (uint32_t j = chars; j > 0; j--) {
                    if (entry.payload[offset + j - 1] == ' ') {
                        chars = j;
                        break;
                    }
                }
            }

            total_lines++;

            offset += chars;

            if (offset < entry.len && entry.payload[offset] == '\n')
                offset++;
        }
    }

    uint32_t first_visible = 0;
    if (total_lines > MAX_LINES) first_visible = total_lines - MAX_LINES;

    uint32_t current_line = 0;
    uint32_t screen_line = 0;

    // Second pass: draw only the visible lines.
    for (uint32_t i = log_buffer_tail; i != log_buffer_head; i = (i + 1) % LOG_MAX_LINES) {
        LogEntry& entry = log_buffer[i];

        uint32_t fgcolor;

        switch (entry.level) {
            case LOG_LEVEL_DEBUG: fgcolor = level_colors[0];
                break;
            case LOG_LEVEL_INFO: fgcolor = level_colors[1];
                break;
            case LOG_LEVEL_WARN: fgcolor = level_colors[2];
                break;
            case LOG_LEVEL_ERROR: fgcolor = level_colors[3];
                break;
            case LOG_LEVEL_FATAL: fgcolor = level_colors[4];
                break;
            default: fgcolor = 0xFFFFFF;
                break;
        }

        uint32_t offset = 0;

        while (offset < entry.len) {
            uint32_t chars = 0;

            while (offset + chars < entry.len &&
                chars < MAX_CHARS &&
                entry.payload[offset + chars] != '\n') {
                chars++;
            }

            if (chars == MAX_CHARS &&
                offset + chars < entry.len &&
                entry.payload[offset + chars] != '\n') {
                for (uint32_t j = chars; j > 0; j--) {
                    if (entry.payload[offset + j - 1] == ' ') {
                        chars = j;
                        break;
                    }
                }
            }

            if (current_line >= first_visible) {
                const char saved = entry.payload[offset + chars];
                entry.payload[offset + chars] = '\0';

                screen::draw(
                    entry.payload + offset,
                    0,
                    screen_line * LINE_HEIGHT,
                    SCALE,
                    fgcolor
                );

                entry.payload[offset + chars] = saved;
                screen_line++;
            }

            current_line++;

            offset += chars;

            if (offset < entry.len && entry.payload[offset] == '\n') offset++;
        }
    }

    screen::flush();
}

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

    redraw();
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

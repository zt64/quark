#include "kernel/stdio.hpp"

#include <kernel/scheduler.hpp>

#include "driver/fb.hpp"
#include "driver/ps2/keyboard.hpp"
#include "lib/string.hpp"

constexpr uint32_t STDIN_SIZE = 1024;
constexpr uint32_t STDOUT_SIZE = 1024;
constexpr uint32_t STDERR_SIZE = 1024;

static uint8_t stdin_buffer[STDIN_SIZE];
static uint8_t stdout_buffer[STDOUT_SIZE];
static uint8_t stderr_buffer[STDERR_SIZE];
static size_t stdin_head = 0;
static size_t stdin_tail = 0;
static size_t stdout_head = 0;
static size_t stdout_tail = 0;
static bool shift_down = false;

namespace {
    char translate_key(const KeyEvent event) {
        if (event.extended_key || event.break_key) {
            return '\0';
        }

        switch (event.scancode) {
            case 0x0D: return '\t';
            case 0x16: return shift_down ? '!' : '1';
            case 0x1E: return shift_down ? '@' : '2';
            case 0x26: return shift_down ? '#' : '3';
            case 0x25: return shift_down ? '$' : '4';
            case 0x2E: return shift_down ? '%' : '5';
            case 0x36: return shift_down ? '^' : '6';
            case 0x3D: return shift_down ? '&' : '7';
            case 0x3E: return shift_down ? '*' : '8';
            case 0x46: return shift_down ? '(' : '9';
            case 0x45: return shift_down ? ')' : '0';
            case 0x4E: return shift_down ? '_' : '-';
            case 0x55: return shift_down ? '+' : '=';
            case 0x66: return '\b';
            case 0x5A: return '\n';
            case 0x29: return ' ';
            case 0x0E: return shift_down ? '~' : '`';
            case 0x15: return shift_down ? 'Q' : 'q';
            case 0x1D: return shift_down ? 'W' : 'w';
            case 0x24: return shift_down ? 'E' : 'e';
            case 0x2D: return shift_down ? 'R' : 'r';
            case 0x2C: return shift_down ? 'T' : 't';
            case 0x35: return shift_down ? 'Y' : 'y';
            case 0x3C: return shift_down ? 'U' : 'u';
            case 0x43: return shift_down ? 'I' : 'i';
            case 0x44: return shift_down ? 'O' : 'o';
            case 0x4D: return shift_down ? 'P' : 'p';
            case 0x54: return shift_down ? '{' : '[';
            case 0x5B: return shift_down ? '}' : ']';
            case 0x5D: return shift_down ? '|' : '\\';
            case 0x1C: return shift_down ? 'A' : 'a';
            case 0x1B: return shift_down ? 'S' : 's';
            case 0x23: return shift_down ? 'D' : 'd';
            case 0x2B: return shift_down ? 'F' : 'f';
            case 0x34: return shift_down ? 'G' : 'g';
            case 0x33: return shift_down ? 'H' : 'h';
            case 0x3B: return shift_down ? 'J' : 'j';
            case 0x42: return shift_down ? 'K' : 'k';
            case 0x4B: return shift_down ? 'L' : 'l';
            case 0x4C: return shift_down ? ':' : ';';
            case 0x52: return shift_down ? '"' : '\'';
            case 0x1A: return shift_down ? 'Z' : 'z';
            case 0x22: return shift_down ? 'X' : 'x';
            case 0x21: return shift_down ? 'C' : 'c';
            case 0x2A: return shift_down ? 'V' : 'v';
            case 0x32: return shift_down ? 'B' : 'b';
            case 0x31: return shift_down ? 'N' : 'n';
            case 0x3A: return shift_down ? 'M' : 'm';
            case 0x41: return shift_down ? '<' : ',';
            case 0x49: return shift_down ? '>' : '.';
            case 0x4A: return shift_down ? '?' : '/';
            default: return '\0';
        }
    }

    void handle_keyboard_event(const KeyEvent event) {
        if (event.scancode == 0x12 || event.scancode == 0x59) {
            shift_down = !event.break_key;
            return;
        }

        const char c = translate_key(event);
        if (c != '\0') {
            stdin_push(static_cast<uint8_t>(c));
        }
    }
}

size_t write_stdin(const uint8_t* data, const size_t count) {
    if (data == nullptr) {
        return 0;
    }

    for (size_t i = 0; i < count; ++i) {
        stdin_push(data[i]);
    }
    return count;
}

static uint32_t line = 0;

void stdout_push(const uint8_t* c) {
    const size_t next = (stdout_head + 1) % STDIN_SIZE;
    if (next == stdout_tail) {
        return;
    }

    stdout_buffer[stdout_head] = *c;
    stdout_head = next;
}

void stderr_push(const uint8_t c) {
}

size_t stdin_read(void* buf, const size_t size) {
    if (buf == nullptr) return 0;

    while (stdin_tail == stdin_head) {
        scheduler::block_current(&stdin_head);
    }

    auto* output = static_cast<uint8_t*>(buf);
    size_t count = 0;
    while (count < size && stdin_tail != stdin_head) {
        output[count++] = stdin_buffer[stdin_tail];
        stdin_tail = (stdin_tail + 1) % STDIN_SIZE;
    }
    return count;
}

void stdin_push(const uint8_t c) {
    const size_t next = (stdin_head + 1) % STDIN_SIZE;
    if (next == stdin_tail) return;

    stdin_buffer[stdin_head] = c;
    stdin_head = next;

    scheduler::unblock(&stdin_head);
}

int stdout_read(void* buf, const size_t size) {
    if (size > STDOUT_SIZE) {
        return -1;
    }

    return 0;
}

int stderr_read(void* buf, const size_t size) {
    if (size > STDERR_SIZE) {
        return -1;
    }

    return 0;
}

void init_stdio() {
    keyboard::register_listener(handle_keyboard_event);
}

#include "lib/format.hpp"
#include "lib/mem.hpp"
#include "lib/stdlib.hpp"
#include <cstdarg>
#include <cstdint>
#include <cwchar>

namespace {
    struct OutBuf {
        char* ptr;
        char* end; // one past last writable byte
    };

    enum flags {
        PAD_ZERO = 1,
        PAD_RIGHT = 2,
    };
}

static void simple_outputchar(OutBuf* out, const char c) {
    if (out) {
        if (out->ptr < out->end) *out->ptr = c;
        ++out->ptr; // always advance so `written` count stays accurate
    }
}

static int prints(OutBuf* out, const char* string, int width, int flags) {
    if (!string) string = "(null)";

    int pc = 0, padchar = ' ';

    if (width > 0) {
        int len = 0;
        for (const char* ptr = string; *ptr; ++ptr) ++len;
        if (len >= width) width = 0;
        else width -= len;
        if (flags & PAD_ZERO) padchar = '0';
    }
    if (!(flags & PAD_RIGHT)) {
        for (; width > 0; --width) {
            simple_outputchar(out, padchar);
            ++pc;
        }
    }
    for (; *string; ++string) {
        simple_outputchar(out, *string);
        ++pc;
    }
    for (; width > 0; --width) {
        simple_outputchar(out, padchar);
        ++pc;
    }

    return pc;
}

#define PRINT_BUF_LEN 64

static int simple_outputi(
    OutBuf* out,
    const int64_t i,
    const int base,
    const int sign,
    int width,
    const int flags,
    const int letbase
) {
    char print_buf[PRINT_BUF_LEN];
    int neg = 0, pc = 0;
    uint64_t u = i;

    if (i == 0) {
        print_buf[0] = '0';
        print_buf[1] = '\0';
        return prints(out, print_buf, width, flags);
    }

    if (sign && base == 10 && i < 0) {
        neg = 1;
        u = -i;
    }

    char* s = print_buf + PRINT_BUF_LEN - 1;
    *s = '\0';

    while (u) {
        int t = u % base;
        if (t >= 10)
            t += letbase - '0' - 10;
        *--s = t + '0';
        u /= base;
    }

    if (neg) {
        if (width && (flags & PAD_ZERO)) {
            simple_outputchar(out, '-');
            ++pc;
            --width;
        } else {
            *--s = '-';
        }
    }

    return pc + prints(out, s, width, flags);
}

int vformat(OutBuf* out, const char* format, va_list ap) {
    int width, flags;
    int pc = 0;
    char scr[2];
    union {
        int8_t i8;
        uint8_t u8;
        int16_t i16;
        uint16_t u16;
        int32_t i32;
        uint32_t u32;
        int64_t i64;
        uint64_t u64;
        char c;
        const char* s;
        void* p;
    } u{};

    for (; *format != 0; ++format) {
        if (*format == '%') {
            ++format;
            width = flags = 0;
            if (*format == '\0')
                break;
            if (*format == '%')
                goto out;
            if (*format == '-') {
                ++format;
                flags = PAD_RIGHT;
            }
            while (*format == '0') {
                ++format;
                flags |= PAD_ZERO;
            }
            if (*format == '*') {
                width = va_arg(ap, int);
                format++;
            } else {
                for (; *format >= '0' && *format <= '9'; ++format) {
                    width *= 10;
                    width += *format - '0';
                }
            }
            switch (*format) {
                case 'd':
                    u.i32 = va_arg(ap, int32_t);
                    pc += simple_outputi(out, u.i32, 10, 1, width, flags, 'a');
                    break;

                case 'u':
                    u.u32 = va_arg(ap, uint32_t);
                    pc += simple_outputi(out, u.u32, 10, 0, width, flags, 'a');
                    break;

                case('x'):
                    u.u32 = va_arg(ap, uint32_t);
                    pc += simple_outputi(out, u.u32, 16, 0, width, flags, 'a');
                    break;

                case('X'):
                    u.u32 = va_arg(ap, uint32_t);
                    pc += simple_outputi(out, u.u32, 16, 0, width, flags, 'A');
                    break;

                case 'z':
                    ++format;
                    switch (*format) {
                        case 'd':
                            u.i64 = va_arg(ap, intptr_t);
                            pc += simple_outputi(out, u.i64, 10, 1, width, flags, 'a');
                            break;

                        case 'u':
                            u.u64 = va_arg(ap, uintptr_t);
                            pc += simple_outputi(out, u.u64, 10, 0, width, flags, 'a');
                            break;

                        case 'x':
                            u.u64 = va_arg(ap, uintptr_t);
                            pc += simple_outputi(out, u.u64, 16, 0, width, flags, 'a');
                            break;

                        case 'X':
                            u.u64 = va_arg(ap, uintptr_t);
                            pc += simple_outputi(out, u.u64, 16, 0, width, flags, 'A');
                            break;

                        default:
                            --format;
                            break;
                    }
                    break;

                case 'p':
                    u.u64 = va_arg(ap, uint64_t);
                    pc += simple_outputi(out, u.u64, 16, 0, width, flags, 'A');
                    break;

                case('c'):
                    u.c = va_arg(ap, int);
                    scr[0] = u.c;
                    scr[1] = '\0';
                    pc += prints(out, scr, width, flags);
                    break;

                case('s'):
                    u.s = va_arg(ap, const char*);
                    pc += prints(out, u.s ? u.s : "(null)", width, flags);
                    break;
                case('l'):
                    ++format;
                    switch (*format) {
                        case('d'):
                            u.i64 = va_arg(ap, int64_t);
                            pc += simple_outputi(out, u.i64, 10, 1, width, flags, 'a');
                            break;

                        case('u'):
                            u.u64 = va_arg(ap, uint64_t);
                            pc += simple_outputi(out, u.u64, 10, 0, width, flags, 'a');
                            break;

                        case('x'):
                            u.u64 = va_arg(ap, uint64_t);
                            pc += simple_outputi(out, u.u64, 16, 0, width, flags, 'a');
                            break;

                        case('X'):
                            u.u64 = va_arg(ap, uint64_t);
                            pc += simple_outputi(out, u.u64, 16, 0, width, flags, 'A');
                            break;

                        case('l'):
                            ++format;
                            switch (*format) {
                                case'd':
                                    u.i64 = va_arg(ap, int64_t);
                                    pc += simple_outputi(out, u.i64, 10, 1, width, flags, 'a');
                                    break;

                                case'u':
                                    u.u64 = va_arg(ap, uint64_t);
                                    pc += simple_outputi(out, u.u64, 10, 0, width, flags, 'a');
                                    break;

                                case'x':
                                    u.u64 = va_arg(ap, uint64_t);
                                    pc += simple_outputi(out, u.u64, 16, 0, width, flags, 'a');
                                    break;

                                case'X':
                                    u.u64 = va_arg(ap, uint64_t);
                                    pc += simple_outputi(out, u.u64, 16, 0, width, flags, 'A');
                                    break;

                                default:
                                    break;
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case('h'):
                    ++format;
                    switch (*format) {
                        case('d'):
                            u.i16 = va_arg(ap, int);
                            pc += simple_outputi(out, u.i16, 10, 1, width, flags, 'a');
                            break;

                        case('u'):
                            u.u16 = va_arg(ap, unsigned int);
                            pc += simple_outputi(out, u.u16, 10, 0, width, flags, 'a');
                            break;

                        case('x'):
                            u.u16 = va_arg(ap, unsigned int);
                            pc += simple_outputi(out, u.u16, 16, 0, width, flags, 'a');
                            break;

                        case('X'):
                            u.u16 = va_arg(ap, unsigned int);
                            pc += simple_outputi(out, u.u16, 16, 0, width, flags, 'A');
                            break;

                        case('h'):
                            ++format;
                            switch (*format) {
                                case('d'):
                                    u.i8 = va_arg(ap, int);
                                    pc += simple_outputi(out, u.i8, 10, 1, width, flags, 'a');
                                    break;

                                case('u'):
                                    u.u8 = va_arg(ap, unsigned int);
                                    pc += simple_outputi(out, u.u8, 10, 0, width, flags, 'a');
                                    break;

                                case('x'):
                                    u.u8 = va_arg(ap, unsigned int);
                                    pc += simple_outputi(out, u.u8, 16, 0, width, flags, 'a');
                                    break;

                                case('X'):
                                    u.u8 = va_arg(ap, unsigned int);
                                    pc += simple_outputi(out, u.u8, 16, 0, width, flags, 'A');
                                    break;

                                default:
                                    break;
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
        } else {
        out:
            simple_outputchar(out, *format);
            ++pc;
        }
    }
    return pc;
}

char* vformat(const char* fmt, va_list ap) {
    size_t cap = 128;
    for (;;) {
        const auto buf = static_cast<char*>(malloc(cap));
        if (!buf) return nullptr;
        OutBuf ob{.ptr = buf, .end = buf + cap};
        va_list ap2;
        va_copy(ap2, ap);
        const int written = vformat(&ob, fmt, ap2);
        va_end(ap2);
        if (written < static_cast<int>(cap)) {
            buf[written] = '\0';
            return buf;
        }
        free(buf);
        cap = static_cast<size_t>(written) + 1;
    }
}

char* format(const char* format, ...) {
    va_list list;
    va_start(list, format);
    char* str = vformat(format, list);
    va_end(list);
    return str;
}

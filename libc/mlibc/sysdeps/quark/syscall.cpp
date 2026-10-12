#include <bits/syscall.h>
#include <errno.h>

extern "C" long __do_syscall_ret(const unsigned long ret) {
    if (static_cast<long>(ret) < 0) {
        errno = static_cast<int>(-ret);
        return -1;
    }
    return static_cast<long>(ret);
}

using sc_word_t = long;

sc_word_t __do_syscall0(long sc) {
    long ret;

    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(sc)
        : "memory", "cc"
    );

    return ret;
}

sc_word_t __do_syscall1(long sc, sc_word_t arg1) {
    long ret;

    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(sc),
        "D"(arg1)
        : "memory", "cc"
    );

    return ret;
}

sc_word_t __do_syscall2(long sc, sc_word_t arg1, sc_word_t arg2) {
    long ret;

    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(sc),
        "D"(arg1),
        "S"(arg2)
        : "memory", "cc"
    );

    return ret;
}

sc_word_t __do_syscall3(
    long sc,
    sc_word_t arg1,
    sc_word_t arg2,
    sc_word_t arg3
) {
    long ret;

    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(sc),
        "D"(arg1),
        "S"(arg2),
        "d"(arg3)
        : "memory", "cc"
    );

    return ret;
}

sc_word_t __do_syscall4(
    long sc,
    sc_word_t arg1,
    sc_word_t arg2,
    sc_word_t arg3,
    sc_word_t arg4
) {
    long ret;

    register sc_word_t r10 asm("r10") = arg4;

    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(sc),
        "D"(arg1),
        "S"(arg2),
        "d"(arg3),
        "r"(r10)
        : "memory", "cc"
    );

    return ret;
}

sc_word_t __do_syscall5(
    long sc,
    sc_word_t arg1,
    sc_word_t arg2,
    sc_word_t arg3,
    sc_word_t arg4,
    sc_word_t arg5
) {
    long ret;

    register sc_word_t r10 asm("r10") = arg4;
    register sc_word_t r8 asm("r8") = arg5;

    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(sc),
        "D"(arg1),
        "S"(arg2),
        "d"(arg3),
        "r"(r10),
        "r"(r8)
        : "memory", "cc"
    );

    return ret;
}

sc_word_t __do_syscall6(
    long sc,
    sc_word_t arg1,
    sc_word_t arg2,
    sc_word_t arg3,
    sc_word_t arg4,
    sc_word_t arg5,
    sc_word_t arg6
) {
    long ret;

    register sc_word_t r10 asm("r10") = arg4;
    register sc_word_t r8 asm("r8") = arg5;
    register sc_word_t r9 asm("r9") = arg6;

    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(sc),
        "D"(arg1),
        "S"(arg2),
        "d"(arg3),
        "r"(r10),
        "r"(r8),
        "r"(r9)
        : "memory", "cc"
    );

    return ret;
}

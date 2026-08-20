#include <bits/syscall.h>
#include <errno.h>

extern "C" long __do_syscall_ret(unsigned long ret) {
    if (static_cast<long>(ret) < 0) {
        errno = static_cast<int>(-ret);
        return -1;
    }
    return static_cast<long>(ret);
}

using sc_word_t = long;

sc_word_t __do_syscall0(long sc) {
    long ret;
    asm volatile ("int $0x80"
        : "=a"(ret)
        : "a"(sc)
        : "memory", "cc");
    return ret;
}

sc_word_t __do_syscall1(long sc, sc_word_t arg1) {
    long ret;
    asm volatile ("int $0x80"
        : "=a"(ret)
        : "a"(sc), "b"(arg1)
        : "memory", "cc");
    return ret;
}

sc_word_t __do_syscall2(long sc, sc_word_t arg1, sc_word_t arg2) {
    long ret;
    asm volatile ("int $0x80"
        : "=a"(ret)
        : "a"(sc), "b"(arg1), "c"(arg2)
        : "memory", "cc");
    return ret;
}

sc_word_t __do_syscall3(long sc, sc_word_t arg1, sc_word_t arg2, sc_word_t arg3) {
    long ret;
    asm volatile ("int $0x80"
        : "=a"(ret)
        : "a"(sc), "b"(arg1), "c"(arg2), "d"(arg3)
        : "memory", "cc");
    return ret;
}

sc_word_t __do_syscall4(long sc, sc_word_t arg1, sc_word_t arg2, sc_word_t arg3, sc_word_t arg4) {
    long ret;
    asm volatile ("int $0x80"
        : "=a"(ret)
        : "a"(sc), "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4)
        : "memory", "cc");
    return ret;
}

sc_word_t __do_syscall5(long sc, sc_word_t arg1, sc_word_t arg2, sc_word_t arg3,
                         sc_word_t arg4, sc_word_t arg5) {
    long ret;
    asm volatile ("int $0x80"
        : "=a"(ret)
        : "a"(sc), "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4), "D"(arg5)
        : "memory", "cc");
    return ret;
}

sc_word_t __do_syscall6(long sc, sc_word_t arg1, sc_word_t arg2, sc_word_t arg3,
                         sc_word_t arg4, sc_word_t arg5, sc_word_t arg6) {
    // Your kernel's handle_syscall only reads rbx/rcx/rdx/rsi/rdi — no 6th argument register.
    // If you need a 6-arg syscall, extend handle_syscall's register set first.
    (void)arg6;
    return __do_syscall5(sc, arg1, arg2, arg3, arg4, arg5);
}
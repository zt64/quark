#include "kernel.hpp"
#include <cstddef>
#include <cstdint>

size_t read(uint32_t fd, void* buf, size_t count) {
    size_t ret;
    asm volatile (
        "int $0x80"
        : "=a" (ret)
        : "a" (0), "b" (fd), "c" (buf), "d" (count)
        : "memory"
    );
    return ret;
}

/**
 *
 * @param fd
 * @param buf
 * @param count How many bytes
 * @return The number of bytes written
 */
size_t write(uint32_t fd, const void* buf, size_t count) {
    size_t ret;
    asm volatile (
        "int $0x80"
        : "=a" (ret)
        : "a" (1), "b" (fd), "c" (buf), "d" (count)
        : "memory"
    );
    return ret;
}

size_t open(const char* pathname, int flags) {
}

size_t close(uint32_t fd) {
    size_t ret;
    asm volatile (
        "int $0x80"
        : "=a" (ret)
        : "a" (3), "b" (fd)
        : "memory"
    );
    return ret;
}

void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
    size_t ret;
    asm volatile (
        "int $0x80"
        : "=a" (ret)
        : "a" (4), "b" (addr), "c" (length), "d" (prot), "S" (flags), "D" (fd), "r" (offset)
        : "memory"
    );
    return reinterpret_cast<void*>(ret);
}

size_t munmap(void* addr, size_t length) {
    size_t ret;
    asm volatile (
        "int $0x80"
        : "=a" (ret)
        : "a" (5), "b" (addr), "c" (length)
        : "memory"
    );
    return ret;
}

size_t exec(const char* pathname, char* const argv[], char* const envp[]) {
    size_t ret;
    asm volatile (
        "int $0x80"
        : "=a" (ret)
        : "a" (6), "b" (pathname), "c" (argv), "d" (envp)
        : "memory"
    );
    return ret;
}

size_t fork() {
    size_t ret;
    asm volatile (
        "int $0x80"
        : "=a" (ret)
        : "a" (7)
        : "memory"
    );
    return ret;
}

size_t getpid() {
    size_t ret;
    asm volatile (
        "int $0x80"
        : "=a" (ret)
        : "a" (8)
        : "memory"
    );
    return ret;
}

size_t sleep(uint32_t milliseconds) {
    size_t ret;
    asm volatile (
        "int $0x80"
        : "=a" (ret)
        : "a" (9), "b" (milliseconds)
        : "memory"
    );
    return ret;
}

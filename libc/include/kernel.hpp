#pragma once

#include <cstddef>
#include <cstdint>
#include <sys/types.h>

constexpr int PROT_READ = 1 << 0;
constexpr int PROT_WRITE = 1 << 1;
constexpr int PROT_EXEC = 1 << 2;
constexpr int MAP_ANON = 1 << 0;

size_t read(uint32_t fd, void* buf, size_t count);
size_t write(uint32_t fd, const void* buf, size_t count);
size_t open(const char* pathname, int flags);
size_t close(uint32_t fd);
void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset);
size_t munmap(void* addr, size_t length);
size_t exec(const char* pathname, char* const argv[], char* const envp[]);
size_t fork();
size_t getpid();
size_t sleep(uint32_t milliseconds);
#include "include/mlibc/sysdeps.hpp"

#include "mlibc/tcb.hpp"
#include <abi-bits/errno.h>
#include <bits/ensure.h>
#include <bits/syscall.h>
#include <mlibc/all-sysdeps.hpp>
#include <string.h>

#include "include/bits/syscall.h"

// Must match the syscall numbers dispatched in kernel/src/kernel/syscall.cpp's handle_syscall().
#define SYS_READ 0
#define SYS_WRITE 1
#define SYS_OPEN 2
#define SYS_CLOSE 3
#define SYS_MMAP 4
#define SYS_MUNMAP 5
#define SYS_EXEC 6
#define SYS_FORK 7
#define SYS_GETPID 8
#define SYS_SLEEP 9
#define SYS_EXIT 10
#define SYS_SET_FS_BASE 11
#define SYS_OPENDIR 12
#define SYS_READDIR 13
#define SYS_CLOSEDIR 14
#define SYS_WAITPID 15

#define STUB()                                                                                     \
	({                                                                                             \
		__ensure(!"STUB function was called");                                                     \
		__builtin_unreachable();                                                                   \
	})

namespace mlibc {
    void Sysdeps<LibcPanic>::operator()() {
        sysdep<LibcLog>("!!! mlibc panic !!!");
        sysdep<Exit>(-1);
        __builtin_trap();
    }

    void Sysdeps<LibcLog>::operator()(const char* msg) {
        ssize_t unused;
        sysdep<Write>(2, msg, strlen(msg), &unused);
    }

    int Sysdeps<Isatty>::operator()(int fd) {
        (void)fd;
        // this returns ENOTTY when it is not a tty, but we do not have a proper implementation
        // so always return that a file is a tty
        return 0;
    }

    int Sysdeps<Write>::operator()(const int fd, void const* buf, size_t size, ssize_t* ret) {
        *ret = syscall(SYS_WRITE, fd, buf, size);
        return 0;
    }

    int Sysdeps<TcbSet>::operator()(void* pointer) {
        // x86_64 TLS (variant II) expects %fs:0 to hold the TCB pointer itself, unlike the
        // RISC-V "mv tp" convention this code was originally copied from. Ask the kernel to
        // set the FS segment base via wrmsr(IA32_FS_BASE, pointer) instead.
        syscall(SYS_SET_FS_BASE, pointer);
        // this can never fail in the demo os
        return 0;
    }

    int Sysdeps<AnonAllocate>::operator()(size_t size, void** pointer) {
        auto out = syscall(
            SYS_MMAP, nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0
        );
        *pointer = (void*)out;
        if (*pointer == MAP_FAILED)
            return ENOMEM; // the syscall does not return a proper errno, so use ENOMEM
        return 0;
    }

    int Sysdeps<AnonFree>::operator()(void*, unsigned long) {
        return 0;
    } // no-op

    int Sysdeps<Seek>::operator()(int, off_t, int, off_t*) {
        return ESPIPE; // no proper file implementation, everything is a tty so return ESPIPE
    }

    void Sysdeps<Exit>::operator()(int status) {
        syscall(SYS_EXIT, status);
        __builtin_unreachable();
    }

    int Sysdeps<GetPid>::operator()() {
        return syscall(SYS_GETPID);
    }

    int Sysdeps<Close>::operator()(int fd) {
        return syscall(SYS_CLOSE, fd);
    }

    int Sysdeps<FutexWake>::operator()(int*, bool) {
        STUB();
    }

    int Sysdeps<FutexWait>::operator()(int*, int, timespec const*) {
        STUB();
    }

    int Sysdeps<Read>::operator()(int fd, void* buf, unsigned long size, long* ret) {
        *ret = syscall(SYS_READ, fd, buf, size);
        return 0;
    }

    int Sysdeps<Open>::operator()(const char*, int, unsigned int, int*) {
        STUB();
    }

    int Sysdeps<OpenDir>::operator()(const char* path, int* handle) {
        syscall(SYS_OPENDIR, path, handle);
        return 0;
    }

    // int Sysdeps<CloseDir>::operator()(const char*, int, unsigned int, int*) {
    //     STUB();
    // }

    int Sysdeps<VmMap>::operator()(void* hint, size_t size, int prot, int flags, int fd, off_t offset, void** window) {
        (void)offset; // kernel's sys_mmap doesn't support file offsets
        auto out = syscall(SYS_MMAP, hint, size, prot, flags, fd);
        if ((void*)out == MAP_FAILED) {
            *window = nullptr;
            return ENOMEM;
        }
        *window = (void*)out;
        return 0;
    }

    int Sysdeps<VmUnmap>::operator()(void* hint, size_t size) {
        auto out = syscall(SYS_MUNMAP, hint, size);
        return 0;
    }

    int Sysdeps<Execve>::operator()(const char *path, char *const argv[], char *const envp[]) {
        syscall(SYS_EXEC, path, argv, envp);
        return 0;
    }

    int Sysdeps<Fork>::operator()(pid_t *child) {
        syscall(SYS_FORK, child);
        return 0;
    }

    int Sysdeps<Waitpid>::operator()(pid_t pid, int *status, int flags, rusage *ru, pid_t *ret_pid) {
        syscall(SYS_WAITPID, pid, status, flags, ru, ret_pid);
        return 0;
    }

    int Sysdeps<ClockGet>::operator()(int, time_t*, long*) {
        STUB();
    }
} // namespace mlibc

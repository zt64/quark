#include "include/mlibc/sysdeps.hpp"

#include "mlibc/tcb.hpp"
#include <abi-bits/errno.h>
#include <bits/ensure.h>
#include <bits/syscall.h>
#include <mlibc/all-sysdeps.hpp>
#include <string.h>

#include "include/bits/syscall.h"

enum class syscall_number : unsigned long {
    read = 0,
    write,
    open,
    close,
    mmap,
    munmap,
    exec,
    fork,
    getpid,
    sleep,
    exit,
    set_fs_base,
    opendir,
    readdir,
    closedir,
    waitpid,
    kill,
    clock_get,
    clock_getres,
    clock_set,
    gettid,
    read_entries,
    stat,
    getcwd,
    chdir,
    mkdir
};

template <typename... Args>
static auto quark_syscall(const syscall_number number, Args... args) {
    return syscall(static_cast<unsigned long>(number), args...);
}

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

        // This returns ENOTTY when it is not a tty, but we do not have a proper
        // implementation, so always return that the file is a tty.
        return 0;
    }

    int Sysdeps<Write>::operator()(
        const int fd,
        void const* buf,
        const size_t size,
        ssize_t* ret
    ) {
        *ret = quark_syscall(syscall_number::write, fd, buf, size);
        return 0;
    }

    int Sysdeps<TcbSet>::operator()(void* pointer) {
        // x86_64 TLS (variant II) expects %fs:0 to hold the TCB pointer itself,
        // unlike the RISC-V "mv tp" convention this code was originally copied
        // from. Ask the kernel to set the FS segment base via
        // wrmsr(IA32_FS_BASE, pointer) instead.
        quark_syscall(syscall_number::set_fs_base, pointer);

        return 0;
    }

    int Sysdeps<AnonAllocate>::operator()(size_t size, void** pointer) {
        auto out = quark_syscall(
            syscall_number::mmap,
            nullptr,
            size,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS,
            -1,
            0
        );

        *pointer = reinterpret_cast<void*>(out);

        if (*pointer == MAP_FAILED)
            return ENOMEM; // The syscall does not return a proper errno yet.

        return 0;
    }

    int Sysdeps<AnonFree>::operator()(void* pointer, const size_t size) {
        quark_syscall(syscall_number::munmap, pointer, size);
        return 0;
    }

    int Sysdeps<Seek>::operator()(int, off_t, int, off_t*) {
        // No proper file implementation yet; everything is a tty.
        return ESPIPE;
    }

    void Sysdeps<Exit>::operator()(int status) {
        quark_syscall(syscall_number::exit, status);
        __builtin_unreachable();
    }

    int Sysdeps<GetPid>::operator()() {
        return quark_syscall(syscall_number::getpid);
    }

    int Sysdeps<Close>::operator()(int fd) {
        return quark_syscall(syscall_number::close, fd);
    }

    int Sysdeps<FutexWake>::operator()(int*, bool) {
        STUB();
    }

    int Sysdeps<FutexWait>::operator()(int*, int, timespec const*) {
        STUB();
    }

    int Sysdeps<Read>::operator()(
        const int fd,
        void* buf,
        const unsigned long size,
        long* ret
    ) {
        *ret = quark_syscall(syscall_number::read, fd, buf, size);
        return 0;
    }

    int Sysdeps<Open>::operator()(const char* pathname, int flags, mode_t mode, int* fd) {
        return quark_syscall(syscall_number::open, pathname, flags, mode, fd);
    }

    int Sysdeps<OpenDir>::operator()(const char* path, int* handle) {
        return quark_syscall(syscall_number::opendir, path, handle);
    }

    int Sysdeps<VmMap>::operator()(
        void* hint,
        const size_t size,
        const int prot,
        const int flags,
        const int fd,
        const off_t offset,
        void** window
    ) {
        (void)offset; // Kernel's sys_mmap doesn't support file offsets.

        const auto out = quark_syscall(
            syscall_number::mmap,
            hint,
            size,
            prot,
            flags,
            fd
        );

        if (reinterpret_cast<void*>(out) == MAP_FAILED) {
            *window = nullptr;
            return ENOMEM;
        }

        *window = reinterpret_cast<void*>(out);
        return 0;
    }

    int Sysdeps<VmUnmap>::operator()(void* pointer, const size_t size) {
        quark_syscall(syscall_number::munmap, pointer, size);
        return 0;
    }

    int Sysdeps<VmProtect>::operator()(void* hint, size_t size, int prot) {
        STUB();
    }

    int Sysdeps<Execve>::operator()(
        const char* path,
        char* const argv[],
        char* const envp[]
    ) {
        return static_cast<int>(quark_syscall(syscall_number::exec, path, argv, envp));
    }
    //
    // int Sysdeps<Yield>::operator()() {
    //     STUB();
    // }

    int Sysdeps<Chdir>::operator()(const char* path) {
        return quark_syscall(syscall_number::chdir, path);
    }

    int Sysdeps<Mkdir>::operator()(const char* path, mode_t mode) {
        return quark_syscall(syscall_number::mkdir, path, mode);
    }

    int Sysdeps<Fork>::operator()(pid_t* child) {
        *child = static_cast<pid_t>(quark_syscall(syscall_number::fork));
        return 0;
    }

    int Sysdeps<Waitpid>::operator()(
        const pid_t pid,
        int* status,
        const int flags,
        rusage* ru,
        pid_t* ret_pid
    ) {
        quark_syscall(
            syscall_number::waitpid,
            pid,
            status,
            flags,
            ru,
            ret_pid
        );

        return 0;
    }

    int Sysdeps<Kill>::operator()(const pid_t pid, const int sig) {
        return quark_syscall(syscall_number::kill, pid, sig);
    }

    int Sysdeps<ClockGet>::operator()(int clock, time_t* secs, long* nanos) {
        return quark_syscall(
            syscall_number::clock_get,
            clock,
            secs,
            nanos
        );
    }

    int Sysdeps<ClockGetres>::operator()(int clock, time_t* secs, long* nanos) {
        return quark_syscall(
            syscall_number::clock_getres,
            clock,
            secs,
            nanos
        );
    }

    int Sysdeps<ClockSet>::operator()(int clock, time_t secs, long nanos) {
        quark_syscall(
            syscall_number::clock_set,
            clock,
            secs,
            nanos
        );
        return 0;
    }

    int Sysdeps<GetTid>::operator()() {
        return quark_syscall(syscall_number::gettid);
    }

    int Sysdeps<ReadEntries>::operator()(
        const int handle,
        void* buffer,
        const size_t max_size,
        size_t* bytes_read
    ) {
        return quark_syscall(
            syscall_number::read_entries,
            handle,
            buffer,
            max_size,
            bytes_read
        );
    }

    int Sysdeps<Stat>::operator()(
        fsfd_target fsfdt,
        int fd,
        const char* path,
        int flags,
        struct stat* statbuf
    ) {
        return quark_syscall(syscall_number::stat, path, statbuf);
    }

    int Sysdeps<GetCwd>::operator()(char* buffer, const size_t size) {
        quark_syscall(syscall_number::getcwd, buffer, size);
        return 0;
    }

    int Sysdeps<Sleep>::operator()(time_t* secs, long* nanos) {
        return quark_syscall(syscall_number::sleep, secs, nanos);
    }
}

#include <cstddef>
#include <type_traits>
#include <bits/utility.h>

#include "../../../include/kernel/syscall/syscall.hpp"
#include "kernel/syscall/internal.hpp"

// Must match the SYS_* defines in libc/mlibc/sysdeps/quark/sysdeps.cpp.
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
    bind,
    listen,
    getsockname,
    getpeername,
    socketpair,
    setsockopt,
    getsockopt,
    clone,
    // fork
    vfork,
    // exit,
    wait4,
    // kill,
    uname,
    semget,
    semop,
    semctl,
    shmdt,
    msgget,
    msgsnd,
    msgrcv,
    msgctl,
    fcntl,
    flock,
    fsync,
    fdatasync,
    truncate,
    ftruncate,
    getdents,
    // getcwd,
    // chdir,
    fchdir,
    rename,
    // mkdir,
    splice,
    tee,
    count,
};

constexpr auto syscall_count = static_cast<int64_t>(syscall_number::count);

using syscall_fn = int64_t(*)(const regs*);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-designator"
static constexpr syscall_fn syscall_table[
    static_cast<int64_t>(syscall_number::count)
] = {
    [static_cast<int64_t>(syscall_number::read)] = handle_read,
    [static_cast<int64_t>(syscall_number::write)] = handle_write,
    [static_cast<int64_t>(syscall_number::open)] = handle_open,
    [static_cast<int64_t>(syscall_number::close)] = handle_close,
    [static_cast<int64_t>(syscall_number::mmap)] = handle_mmap,
    [static_cast<int64_t>(syscall_number::munmap)] = handle_munmap,
    [static_cast<int64_t>(syscall_number::exec)] = handle_exec,
    [static_cast<int64_t>(syscall_number::fork)] = handle_fork,
    [static_cast<int64_t>(syscall_number::getpid)] = handle_getpid,
    [static_cast<int64_t>(syscall_number::sleep)] = handle_sleep,
    [static_cast<int64_t>(syscall_number::exit)] = handle_exit,
    [static_cast<int64_t>(syscall_number::set_fs_base)] = handle_set_fs_base,
    [static_cast<int64_t>(syscall_number::opendir)] = handle_opendir,
    [static_cast<int64_t>(syscall_number::readdir)] = handle_readdir,
    [static_cast<int64_t>(syscall_number::closedir)] = handle_closedir,
    [static_cast<int64_t>(syscall_number::waitpid)] = handle_waitpid,
    [static_cast<int64_t>(syscall_number::kill)] = handle_kill,
    [static_cast<int64_t>(syscall_number::clock_get)] = nullptr,
    [static_cast<int64_t>(syscall_number::clock_getres)] = nullptr,
    [static_cast<int64_t>(syscall_number::clock_set)] = nullptr,
    [static_cast<int64_t>(syscall_number::gettid)] = handle_gettid,
    [static_cast<int64_t>(syscall_number::read_entries)] = handle_readentries,
    [static_cast<int64_t>(syscall_number::stat)] = handle_getstat,
    [static_cast<int64_t>(syscall_number::getcwd)] = handle_getcwd,
    [static_cast<int64_t>(syscall_number::chdir)] = handle_chdir,
};
#pragma clang diagnostic pop

extern "C" void handle_syscall(regs* r) {
    const auto n = r->rax;

    if (n >= static_cast<int64_t>(syscall_number::count) || syscall_table[n] == nullptr) {
        r->rax = static_cast<uint64_t>(-1);
        return;
    }

    r->rax = syscall_table[n](r);
}

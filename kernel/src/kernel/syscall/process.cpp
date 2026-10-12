#include <driver/timer.hpp>
#include <kernel/log.hpp>
#include <kernel/platform.hpp>
#include <kernel/process/process.hpp>
#include <kernel/process/scheduler.hpp>
#include <kernel/process/userspace.hpp>
#include <kernel/syscall/syscall.hpp>
#include <memory/paging.hpp>
#include "arch/msr.hpp"
#include "kernel/syscall/internal.hpp"

[[noreturn]] static void sys_exec(const char* path, char** argv, char** envp) {
    if (!path) {
        logger.error("sys_exec: path is null");
        panic("sys_exec: path is null");
    }

    platform::disable_interrupts();

    const process_t* p = userspace::exec(current, path, argv, envp);
    if (p == nullptr) {
        platform::enable_interrupts();
        panic("sys_exec: failed to exec %s", path);
    }

    process::switch_to_task(p->main_task);

    panic("sys_exec: resumed after switch_to_task - unreachable");
}

static size_t sys_fork() {
    const process_t* child = process::fork_current(current);
    return child->pid;
}

static size_t sys_getpid() {
    return current->proc->pid;
}

static size_t sys_sleep(const uint32_t* seconds, const uint32_t* nanoseconds) {
    if (!seconds || !nanoseconds) {
        logger.error("sys_sleep: seconds or nanoseconds is null");
        return -1;
    }

    const uint32_t sec = *seconds;
    const uint32_t nsec = *nanoseconds;

    if (sec == 0) {
        return 0;
    }

    logger.debug("Task tid=%d sleeping for %u seconds and %u nanoseconds", current->tid, sec, nsec);

    timer::sleep_ms(sec);

    return sec;
}

[[noreturn]] static void sys_exit(const int status) {
    task* t = current;
    t->state = ZOMBIE;

    logger.debug("Task tid=%d exited with status %d", t->tid, status);

    scheduler::terminate_proc(t->proc);

    // terminate_proc() removed us from the run queue and woke any waiting
    // parent, but we're still "current" and still executing on our own
    // (now-unreachable) kernel stack -- hand off to another task instead of
    // ever letting control fall back through handle_syscall/isr_dispatch,
    // which would iretq straight back into this exited task's userspace.
    scheduler::exit_current();
}

constexpr uint32_t IA32_FS_BASE = 0xC0000100;

static size_t sys_set_fs_base(const uint64_t base) {
    current->fs_base = base;
    const auto lo = static_cast<uint32_t>(base);
    const auto hi = static_cast<uint32_t>(base >> 32);
    arch::cpu_set_msr(IA32_FS_BASE, lo, hi);
    return 0;
}

static size_t sys_waitpid(const int pid, int* status, const int options, void* ru, void* ret_pid) {
    (void)status;
    (void)options;
    (void)ru;
    (void)ret_pid;

    // TODO: Check if PID is valid

    scheduler::block_current(reinterpret_cast<const void*>(pid));

    return 0;
}

static size_t sys_kill(const int pid, const int sig) {
    if (!pid) {
        logger.error("sys_kill: invalid pid %d", pid);
        return -1;
    }

    //
    // task* t = process::find_task_by_pid(pid);
    // if (!t) {
    //     logger.error("sys_kill: no task with pid %d", pid);
    //     return -1;
    // }
    //
    // t->state = ZOMBIE;
    // scheduler::terminate_proc(t->proc);

    return 0;
}

int64_t handle_exec(const regs* r) {
    invoke_syscall(sys_exec, r);

    __builtin_unreachable();
}

SYSCALL_HANDLER(fork);
SYSCALL_HANDLER(getpid);
SYSCALL_HANDLER(sleep);
SYSCALL_HANDLER(exit);
SYSCALL_HANDLER(set_fs_base);
SYSCALL_HANDLER(waitpid);
SYSCALL_HANDLER(kill);

int64_t handle_gettid(const regs*) {
    // TODO: Implement
    __builtin_unreachable();
}

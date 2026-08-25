#include "kernel/syscall.hpp"
#include "kernel/system.hpp"
#include <cstddef>
#include <lib/mem.hpp>
#include <driver/fb.hpp>
#include <driver/timer.hpp>
#include <driver/fs/vfs.hpp>
#include <kernel/log.hpp>
#include <kernel/process.hpp>
#include <kernel/scheduler.hpp>
#include <kernel/userspace.hpp>
#include <lib/printf.h>
#include <memory/paging.hpp>
#include <memory/pmm.hpp>
#include <memory/vmm.hpp>
#include "arch/msr.hpp"
#include "kernel/stdio.hpp"

// Must match the SYS_* defines in libc/mlibc/sysdeps/quark/sysdeps.cpp.
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
#define SYS_COUNT 16

// ---- syscall implementations (unchanged) ----

static size_t sys_read(const uint32_t fd, void* buf, const size_t count) {
    if (buf == nullptr) {
        return 0;
    }
    switch (fd) {
        case 0:
            return stdin_read(buf, count);
        case 1:
            return stdout_read(buf, count);
        default:
            return 0;
    }
}

static size_t sys_write(const uint32_t fd, const void* buf, const size_t count) {
    switch (fd) {
        case 0:
            if (buf && count > 0) {
                return write_stdin(static_cast<const uint8_t*>(buf), count);
            }
            break;
        case 1:
            if (buf && count > 0) {
                stdout_push(static_cast<const uint8_t*>(buf));
                printf(static_cast<const char*>(buf));
                return count;
            }
            break;
        case 2:
            if (buf && count > 0) {
                stdout_push(static_cast<const uint8_t*>(buf));
                logger.error(static_cast<const char*>(buf), count);
                return count;
            }
        default:
            break;
    }
    return 0;
}

static size_t sys_open(const uint32_t fd, const uint32_t flags) {
    (void)fd;
    (void)flags;
    return 0;
}

static size_t sys_close(const uint32_t fd) {
    (void)fd;
    return 0;
}

static size_t align_up(const size_t v, const size_t align) {
    return (v + (align - 1)) & ~(align - 1);
}

constexpr int MMAP_FD_FRAMEBUFFER = 3;

static void* sys_mmap(
    const void* addr, const size_t length, const int prot, const int flags, const int fd
) {
    (void)prot;
    (void)flags;

    if (length == 0) {
        return nullptr;
    }

    uintptr_t user_base;

    if (addr == nullptr) {
        user_base = vmm::find_free_addr(current->cr3, length);
    } else {
        user_base = reinterpret_cast<uintptr_t>(addr);
    }

    if (fd == MMAP_FD_FRAMEBUFFER) {
        const auto fb_virt = reinterpret_cast<uintptr_t>(screen::framebuffer.addr);

        uintptr_t fb_phys;
        if (!paging::translate(fb_virt, fb_phys)) {
            return nullptr;
        }

        const size_t size = screen::framebuffer.pitch * screen::framebuffer.height;
        const uintptr_t phys_page = fb_phys & ~(paging::PAGE_SIZE - 1);
        const size_t page_offset = fb_phys & (paging::PAGE_SIZE - 1);
        const size_t map_size = align_up(size + page_offset, paging::PAGE_SIZE);

        for (size_t map_offset = 0; map_offset < map_size; map_offset += paging::PAGE_SIZE) {
            paging::map_page(
                current->cr3,
                user_base + map_offset,
                phys_page + map_offset,
                paging::PAGE_USER | paging::PAGE_WRITABLE);
        }

        vmm::track(current->cr3, user_base, map_size, VM_FLAG_WRITE | VM_FLAG_USER);

        return reinterpret_cast<void*>(user_base + page_offset);
    }

    const size_t map_size = align_up(length, paging::PAGE_SIZE);
    const size_t page_count = map_size / paging::PAGE_SIZE;

    const void* phys = mem::allocate_physical_pages(page_count);
    if (!phys) {
        return nullptr;
    }

    const auto phys_base = reinterpret_cast<uintptr_t>(phys);

    for (size_t map_offset = 0; map_offset < map_size; map_offset += paging::PAGE_SIZE) {
        if (!paging::map_page(
            current->cr3,
            user_base + map_offset,
            phys_base + map_offset,
            paging::PAGE_USER | paging::PAGE_WRITABLE)) {
            logger.error("sys_mmap: failed to map page at 0x%lx", user_base + map_offset);
        }
    }

    vmm::track(current->cr3, user_base, map_size, VM_FLAG_WRITE | VM_FLAG_USER);

    return reinterpret_cast<void*>(user_base);
}

static size_t sys_munmap(const void* addr, const size_t length, const int prot, const int flags) {
    (void)addr;
    (void)length;
    (void)prot;
    (void)flags;
    return 0;
}

static size_t sys_exec(const char* path, char* const argv[], char* const envp[]) {
    userspace::launch(path, argv, envp, nullptr);
    return 0;
}

// TODO: Kernel stack size should be defined in a constant somewhere
constexpr uint8_t KERNEL_STACK_PAGES = 4;

static size_t sys_fork() {
    panic("sys_fork is not implemented cause i am too incompetant to figure out how");
}

static size_t sys_getpid() {
    return current->pid;
}

static size_t sys_sleep(const uint32_t milliseconds) {
    asm volatile("sti");
    timer::wait_ms(milliseconds);
    asm volatile("cli");
    return 0;
}

[[noreturn]] static void sys_exit(const int status) {
    task* t = current;
    t->state = ZOMBIE;

    logger.debug("Task pid=%d exited with status %d", t->pid, status);

    task* it = t->next;
    while (it != t && it->next != t) {
        it = it->next;
    }
    if (it != t) {
        it->next = t->next;
    }

    scheduler::block_current(nullptr);
    __builtin_unreachable();
}

constexpr uint32_t IA32_FS_BASE = 0xC0000100;

static size_t sys_set_fs_base(const uint64_t base) {
    current->fs_base = base;
    const auto lo = static_cast<uint32_t>(base);
    const auto hi = static_cast<uint32_t>(base >> 32);
    arch::cpu_set_msr(IA32_FS_BASE, lo, hi);
    return 0;
}

static size_t sys_opendir(const char* path, int* handle) {
    (void)handle;

    vfs::opendir(path);

    return static_cast<size_t>(-1);
}

static size_t sys_readdir(const int handle, void* dirent_buf) {
    (void)handle;
    (void)dirent_buf;
    return static_cast<size_t>(-1);
}

static size_t sys_closedir(const int handle) {
    return vfs::closedir(handle);
}

static size_t sys_waitpid(const int pid, int* status, const int options, void* ru, void* ret_pid) {
    (void)status;
    (void)options;
    (void)ru;
    (void)ret_pid;

    scheduler::block_current(reinterpret_cast<const void*>(pid));

    return static_cast<size_t>(-1);
}

static uint64_t handle_read(const regs* r) {
    return sys_read(r->rbx, reinterpret_cast<void*>(r->rcx), r->rdx);
}

static uint64_t handle_write(const regs* r) {
    return sys_write(r->rbx, reinterpret_cast<const void*>(r->rcx), r->rdx);
}

static uint64_t handle_open(const regs* r) {
    return sys_open(r->rbx, r->rcx);
}

static uint64_t handle_close(const regs* r) {
    return sys_close(r->rbx);
}

static uint64_t handle_mmap(const regs* r) {
    return reinterpret_cast<uint64_t>(
        sys_mmap(reinterpret_cast<void*>(r->rbx), r->rcx, r->rdx, r->rsi, r->rdi)
    );
}

static uint64_t handle_munmap(const regs* r) {
    return sys_munmap(reinterpret_cast<void*>(r->rbx), r->rcx, r->rdx, r->rsi);
}

static uint64_t handle_exec(const regs* r) {
    return sys_exec(
        reinterpret_cast<const char*>(r->rbx),
        reinterpret_cast<char* const*>(r->rcx),
        reinterpret_cast<char* const*>(r->rdx)
    );
}

static uint64_t handle_fork(const regs*) {
    return sys_fork();
}

static uint64_t handle_getpid(const regs*) {
    return sys_getpid();
}

static uint64_t handle_sleep(const regs* r) {
    return sys_sleep(r->rbx);
}

static uint64_t handle_exit(const regs* r) {
    sys_exit(static_cast<int>(r->rbx));
    __builtin_unreachable();
}

static uint64_t handle_set_fs_base(const regs* r) {
    return sys_set_fs_base(r->rbx);
}

static uint64_t handle_opendir(const regs* r) {
    return sys_opendir(
        reinterpret_cast<const char*>(r->rbx),
        reinterpret_cast<int*>(r->rcx)
    );
}

static uint64_t handle_readdir(const regs* r) {
    return sys_readdir(
        static_cast<int>(r->rbx),
        reinterpret_cast<void*>(r->rcx)
    );
}

static uint64_t handle_closedir(const regs* r) {
    return sys_closedir(static_cast<int>(r->rbx));
}

static uint64_t handle_waitpid(const regs* r) {
    return sys_waitpid(
        static_cast<int>(r->rbx),
        reinterpret_cast<int*>(r->rcx),
        static_cast<int>(r->rdx),
        reinterpret_cast<void*>(r->rsi),
        reinterpret_cast<void*>(r->rdi)
    );
}
// ---- dispatch table ----

using syscall_fn = uint64_t(*)(const regs*);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-designator"
static constexpr syscall_fn syscall_table[SYS_COUNT] = {
    [SYS_READ] = handle_read,
    [SYS_WRITE] = handle_write,
    [SYS_OPEN] = handle_open,
    [SYS_CLOSE] = handle_close,
    [SYS_MMAP] = handle_mmap,
    [SYS_MUNMAP] = handle_munmap,
    [SYS_EXEC] = handle_exec,
    [SYS_FORK] = handle_fork,
    [SYS_GETPID] = handle_getpid,
    [SYS_SLEEP] = handle_sleep,
    [SYS_EXIT] = handle_exit,
    [SYS_SET_FS_BASE] = handle_set_fs_base,
    [SYS_OPENDIR] = handle_opendir,
    [SYS_READDIR] = handle_readdir,
    [SYS_CLOSEDIR] = handle_closedir,
    [SYS_WAITPID] = handle_waitpid
};
#pragma clang diagnostic pop

extern "C" void handle_syscall(regs* r) {
    logger.debug("handle_syscall: pid=%d syscall=%lu rbx=0x%lx rcx=0x%lx rdx=0x%lx rsi=0x%lx rdi=0x%lx",
        current->pid, r->rax, r->rbx, r->rcx, r->rdx, r->rsi, r->rdi);

    const uint32_t n = r->rax;

    if (n >= SYS_COUNT || syscall_table[n] == nullptr) {
        r->rax = static_cast<uint64_t>(-1);
        return;
    }

    r->rax = syscall_table[n](r);
}

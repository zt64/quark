#include "kernel/syscall.hpp"

#include "kernel/system.hpp"
#include <cstddef>
#include <lib/mem.hpp>
#include <driver/fb.hpp>
#include <driver/timer.hpp>
#include <kernel/log.hpp>
#include <kernel/process.hpp>
#include <kernel/scheduler.hpp>
#include <kernel/userspace.hpp>
#include <memory/paging.hpp>
#include <memory/vmm.hpp>
#include "kernel/stdio.hpp"

static size_t sys_read(const uint32_t fd, void* buf, const size_t count) {
    if (buf == nullptr) {
        return 0;
    }

    switch (fd) {
        case 0:
            return stdin_read(buf, count);
        case 1:
            return stdout_read(buf, count);
        default: // TODO: Handle fd
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
                logger.debug(static_cast<const char*>(buf), count);
                return count;
            }
            break;
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

static void* sys_mmap(
    const void* addr, const size_t length, const int prot, const int flags, const int fd
) {
    // sys_mmap: fd == 3 means framebuffer
    uintptr_t user_base;

    if (addr == nullptr) {
        user_base = vmm::find_free_addr(length);
    } else {
        user_base = reinterpret_cast<uintptr_t>(addr);
    }

    const uintptr_t fb_virt = reinterpret_cast<uintptr_t>(screen::framebuffer.addr);

    uintptr_t fb_phys;
    if (!paging::translate(fb_virt, fb_phys)) {
        return nullptr;
    }

    const size_t size = screen::framebuffer.pitch * screen::framebuffer.height;
    const uintptr_t phys_page = fb_phys & ~(paging::PAGE_SIZE - 1);
    const size_t page_offset = fb_phys & (paging::PAGE_SIZE - 1);
    const size_t map_size = align_up(size + page_offset, paging::PAGE_SIZE);

    // Reserve user virtual space, but map phys_page + offset rather than allocating pages.
    for (size_t map_offset = 0; map_offset < map_size; map_offset += paging::PAGE_SIZE) {
        paging::map_page(
            current->cr3,
            user_base + map_offset,
            phys_page + map_offset,
            paging::PAGE_USER | paging::PAGE_WRITABLE);
    }

    return reinterpret_cast<void*>(user_base + page_offset);
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
}

static size_t sys_fork() {
    const task* parent = current;

    task* child_task = create_task(parent->module_addr);

    child_task->parent = const_cast<task*>(parent);

    for (uintptr_t offset = 0; offset < parent->stack_size; offset += paging::PAGE_SIZE) {
        uintptr_t parent_phys;
        if (!paging::translate(parent->cr3, parent->stack_base + offset, parent_phys)) {
            panic("fork: failed to translate parent stack page");
        }

        uintptr_t child_phys;
        if (!paging::translate(child_task->cr3, child_task->stack_base + offset, child_phys)) {
            panic("fork: failed to translate child stack page");
        }

        const auto* src = reinterpret_cast<const void*>(parent_phys + paging::g_hhdm_offset);
        auto* dst = reinterpret_cast<void*>(child_phys + paging::g_hhdm_offset);
        memcpy(dst, src, paging::PAGE_SIZE);
    }
    constexpr uintptr_t KERNEL_STACK_SIZE = 4 * paging::PAGE_SIZE;

    const uintptr_t parent_kernel_base =
        reinterpret_cast<uintptr_t>(parent->rsp0) - KERNEL_STACK_SIZE;

    const uintptr_t child_kernel_base =
        reinterpret_cast<uintptr_t>(child_task->rsp0) - KERNEL_STACK_SIZE;

    for (uintptr_t offset = 0; offset < KERNEL_STACK_SIZE; offset += paging::PAGE_SIZE) {
        uintptr_t parent_phys;
        if (!paging::translate(parent->cr3, parent_kernel_base + offset, parent_phys)) {
            panic("fork: failed to translate parent kernel stack page");
        }

        uintptr_t child_phys;
        if (!paging::translate(child_task->cr3, child_kernel_base + offset, child_phys)) {
            panic("fork: failed to translate child kernel stack page");
        }

        memcpy(
            reinterpret_cast<void*>(child_phys + paging::g_hhdm_offset),
            reinterpret_cast<const void*>(parent_phys + paging::g_hhdm_offset),
            paging::PAGE_SIZE
        );
    }

    // Rebase scheduler context.
    const uintptr_t rsp_offset =
        reinterpret_cast<uintptr_t>(parent->rsp) - parent_kernel_base;

    child_task->rsp =
        reinterpret_cast<void*>(child_kernel_base + rsp_offset);

    // Rebase trap frame.
    const uintptr_t trap_offset =
        reinterpret_cast<uintptr_t>(parent->trap_frame) - parent_kernel_base;

    child_task->trap_frame =
        reinterpret_cast<regs*>(child_kernel_base + trap_offset);

    // Child returns 0 from fork().
    process::write_kernel_qword(
        child_task->cr3,
        reinterpret_cast<uintptr_t>(child_task->trap_frame) + offsetof(regs, rax),
        0
    );
    current->next = child_task;
    child_task->next = const_cast<task*>(parent);
    scheduler::reschedule();
    logger.debug("forked new task with pid: %d", child_task->pid);
    return child_task->pid;
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

extern "C" void handle_syscall(regs* r) {
    const uint32_t syscall_number = r->rax;

    switch (syscall_number) {
        case 0:
            r->rax = sys_read(r->rbx, reinterpret_cast<void*>(r->rcx), r->rdx);
            break;
        case 1:
            r->rax = sys_write(r->rbx, reinterpret_cast<const void*>(r->rcx), r->rdx);
            break;
        case 2:
            r->rax = sys_open(r->rbx, r->rcx);
            break;
        case 3:
            //close
            r->rax = sys_close(r->rbx);
            break;
        case 4:
            r->rax = reinterpret_cast<uint64_t>(
                sys_mmap(reinterpret_cast<void*>(r->rbx), r->rcx, r->rdx, r->rsi, r->rdi)
            );
            break;
        case 5:
            r->rax = sys_munmap(reinterpret_cast<void*>(r->rbx), r->rcx, r->rdx, r->rsi);
            break;
        case 6:
            r->rax = sys_exec(
                reinterpret_cast<const char*>(r->rbx),
                reinterpret_cast<char* const*>(r->rcx),
                reinterpret_cast<char* const*>(r->rdx)
            );
            break;
        case 7:
            r->rax = sys_fork();
            break;
        case 8:
            r->rax = sys_getpid();
            break;
        case 9:
            r->rax = sys_sleep(r->rbx);
            break;
        default:
            r->rax = static_cast<uint64_t>(-1);
            break;
    }
}

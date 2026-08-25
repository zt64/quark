#include "kernel/process.hpp"

#include <arch/msr.hpp>
#include <kernel/elf.hpp>
#include <kernel/log.hpp>
#include <kernel/scheduler.hpp>
#include <kernel/system.hpp>
#include <kernel/tss.hpp>
#include <kernel/userspace.hpp>
#include <memory/paging.hpp>
#include <memory/vmm.hpp>

constexpr uint32_t PID_MAX = UINT16_MAX;
uint32_t pid = 1;
task* current = nullptr;
task* processes[PID_MAX] = {};
constexpr uint32_t stack_pages = 64;
constexpr uint32_t kernel_stack_pages = 4;

extern "C" void switch_context(uint64_t* old_rsp_out, uint64_t new_rsp);

namespace process {
    [[noreturn]]
    void task_bootstrap() {
        userspace::enter_task(current);
        __builtin_unreachable();
    }

    void write_kernel_qword(const uint64_t cr3, const uintptr_t virt_addr, const uint64_t value) {
        uintptr_t phys_addr = 0;
        if (!paging::translate(cr3, virt_addr, phys_addr)) {
            panic("Unable to translate kernel stack address");
        }
        *reinterpret_cast<uint64_t*>(phys_addr + paging::g_hhdm_offset) = value;
    }

    static void* build_kernel_frame(const uint64_t cr3, const uintptr_t kernel_stack_top) {
        uintptr_t frame_top = kernel_stack_top;
        write_kernel_qword(
            cr3,
            frame_top -= sizeof(uint64_t),
            reinterpret_cast<uint64_t>(task_bootstrap)
        );
        write_kernel_qword(cr3, frame_top -= sizeof(uint64_t), 0); // rbx
        write_kernel_qword(cr3, frame_top -= sizeof(uint64_t), 0); // rbp
        write_kernel_qword(cr3, frame_top -= sizeof(uint64_t), 0); // r12
        write_kernel_qword(cr3, frame_top -= sizeof(uint64_t), 0); // r13
        write_kernel_qword(cr3, frame_top -= sizeof(uint64_t), 0); // r14
        write_kernel_qword(cr3, frame_top -= sizeof(uint64_t), 0); // r15
        return reinterpret_cast<void*>(frame_top);
    }
}

task* create_task(const uintptr_t module_addr) {
    const auto ret = new task();
    const uint64_t cr3 = paging::new_address_space();
    const auto* img = elf::load_file(cr3, module_addr);

    constexpr uint32_t stack_size = stack_pages * paging::PAGE_SIZE;
    const uintptr_t stack_base = vmm::vmm_alloc(
        cr3,
        stack_pages * paging::PAGE_SIZE,
        VM_FLAG_USER | VM_FLAG_WRITE,
        nullptr
    );

    if (stack_base == 0) {
        panic("Unable to allocate user-mode stack page");
    }

    const uintptr_t kernel_stack_base = vmm::vmm_alloc(
        cr3,
        kernel_stack_pages * paging::PAGE_SIZE,
        VM_FLAG_WRITE,
        nullptr
    );

    if (kernel_stack_base == 0) {
        panic("Unable to allocate kernel stack page");
    }

    if (pid >= PID_MAX) {
        panic("create_task: pid table exhausted");
    }

    ret->pid = pid++;
    ret->cr3 = cr3;
    logger.debug("create_task: pid=%d cr3=0x%lx", ret->pid, cr3);
    ret->rsp0 = reinterpret_cast<void*>(kernel_stack_base + kernel_stack_pages * paging::PAGE_SIZE);
    ret->rsp = process::build_kernel_frame(cr3, reinterpret_cast<uintptr_t>(ret->rsp0));
    ret->entry_point = img->entry_point;
    ret->phdr_addr = img->phdr_addr;
    ret->phentsize = img->phentsize;
    ret->phnum = img->phnum;
    ret->stack_base = stack_base;
    ret->stack_size = stack_size;
    ret->module_addr = module_addr;
    ret->next = ret;
    ret->state = RUNNING;

    // Splice into the circular scheduler list without dropping existing
    // tasks: point the new task at whatever `current` used to point to,
    // then have `current` point at the new task.
    if (current != nullptr) {
        ret->next = current->next;
        current->next = ret;
    } else {
        ret->next = ret;
    }

    processes[ret->pid] = ret;

    return ret;
}


void terminate_task(const task* t) {
    logger.debug("Terminating task: pid=%d cr3=0x%lx", t->pid, t->cr3);

    // TODO: kill children first

    processes[t->pid] = nullptr;
    paging::free_address_space(t->cr3);
    scheduler::unblock(reinterpret_cast<const void*>(t->pid));
    scheduler::reschedule();
}

#include "kernel/process.hpp"

#include <kernel/elf.hpp>
#include <kernel/log.hpp>
#include <kernel/system.hpp>
#include <kernel/tss.hpp>
#include <kernel/userspace.hpp>
#include <memory/paging.hpp>
#include <memory/vmm.hpp>

constexpr uint32_t PID_MAX = UINT16_MAX;
uint32_t pid = 1;
task* current = nullptr;
task* processes[PID_MAX];

constexpr uint32_t stack_pages = 4;
constexpr uint32_t kernel_stack_pages = 4;

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

    void* build_kernel_frame(const uint64_t cr3, const uintptr_t kernel_stack_top) {
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

    // Runs on the *next* task's stack, after the switch. Handles the
    // address-space change and TSS update — safe to do in C++ here
    // because rsp already points into valid, next-task-owned memory.
    extern "C" void finish_task_switch(task* next) {
        uint64_t current_cr3;
        asm volatile("mov %%cr3, %0" : "=r"(current_cr3));

        if (current_cr3 != next->cr3) {
            asm volatile("mov %0, %%cr3" :: "r"(next->cr3) : "memory");
        }

        tss::set_kernel_stack(reinterpret_cast<uintptr_t>(next->rsp0));
    }
}

extern "C" task* context_switch_prepare(task* next, void* old_rsp) {
    if (current)
        current->rsp = old_rsp;

    task* prev = current;
    current = next;
    return prev;
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

    ret->pid = pid++;
    ret->cr3 = cr3;
    ret->rsp0 = reinterpret_cast<void*>(kernel_stack_base + kernel_stack_pages * paging::PAGE_SIZE);
    ret->rsp = process::build_kernel_frame(cr3, reinterpret_cast<uintptr_t>(ret->rsp0) + paging::g_hhdm_offset);
    ret->entry_point = img->entry_point;
    ret->stack_base = stack_base;
    ret->stack_size = stack_size;
    ret->module_addr = module_addr;
    ret->next = ret;
    ret->state = RUNNING;
    return ret;
}
#include "kernel/process/task.hpp"
#include <arch/msr.hpp>
#include <arch/x86_64/tss.hpp>
#include <kernel/system.hpp>
#include <kernel/process/userspace.hpp>
#include <memory/paging.hpp>

task* current = nullptr;

extern "C" void isr_return();

namespace process {
    void switch_to_task(task* const next_thread) {
        tss::set_kernel_stack(reinterpret_cast<uintptr_t>(next_thread->rsp0));

        arch::cpu_set_msr(
            arch::IA32_FS_BASE,
            static_cast<uint32_t>(next_thread->fs_base),
            static_cast<uint32_t>(next_thread->fs_base >> 32) // upper 32 bits
        );

        task* const prev = current;
        current = next_thread;

        void** const prev_rsp_slot = (prev != nullptr && prev != next_thread) ? &prev->rsp : nullptr;

        raw_switch_to_task(prev_rsp_slot, next_thread->rsp, next_thread->cr3);
    }

    void write_kqword(const uint64_t cr3, const uintptr_t virt_addr, const uint64_t value) {
        uintptr_t phys_addr = 0;
        if (!paging::translate(cr3, virt_addr, phys_addr)) {
            panic("Unable to translate kernel stack address");
        }
        *reinterpret_cast<uint64_t*>(phys_addr + paging::g_hhdm_offset) = value;
    }

    [[noreturn]]
    void task_bootstrap() {
        userspace::enter_task(current);
        __builtin_unreachable();
    }

    void* build_kernel_frame(const uint64_t cr3, const uintptr_t kernel_stack_top) {
        uintptr_t frame_top = kernel_stack_top;
        write_kqword(
            cr3,
            frame_top -= sizeof(uint64_t),
            reinterpret_cast<uint64_t>(task_bootstrap)
        );
        write_kqword(cr3, frame_top -= sizeof(uint64_t), 0); // rbx
        write_kqword(cr3, frame_top -= sizeof(uint64_t), 0); // rbp
        write_kqword(cr3, frame_top -= sizeof(uint64_t), 0); // r12
        write_kqword(cr3, frame_top -= sizeof(uint64_t), 0); // r13
        write_kqword(cr3, frame_top -= sizeof(uint64_t), 0); // r14
        write_kqword(cr3, frame_top -= sizeof(uint64_t), 0); // r15
        write_kqword(cr3, frame_top -= sizeof(uint64_t), 0); // rax
        return reinterpret_cast<void*>(frame_top);
    }

    void* build_fork_frame(const uint64_t cr3, const uintptr_t trap_frame_addr) {
        uintptr_t frame_top = trap_frame_addr;
        write_kqword(cr3, frame_top -= sizeof(uint64_t), reinterpret_cast<uint64_t>(isr_return));
        write_kqword(cr3, frame_top -= sizeof(uint64_t), 0); // rbx
        write_kqword(cr3, frame_top -= sizeof(uint64_t), 0); // rbp
        write_kqword(cr3, frame_top -= sizeof(uint64_t), 0); // r12
        write_kqword(cr3, frame_top -= sizeof(uint64_t), 0); // r13
        write_kqword(cr3, frame_top -= sizeof(uint64_t), 0); // r14
        write_kqword(cr3, frame_top -= sizeof(uint64_t), 0); // r15
        write_kqword(cr3, frame_top -= sizeof(uint64_t), 0); // rax
        return reinterpret_cast<void*>(frame_top);
    }
}

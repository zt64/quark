#pragma once

#include <kernel/system.hpp>

/**
 * Task state
 */
enum task_state {
    RUNNING,
    SLEEPING,
    STOPPED,
    ZOMBIE
};

struct process_t;

struct task {
    uint32_t tid;
    uint32_t _pad;
    void* rsp;
    void* rsp0;
    uint64_t cr3; // cached copy of proc->cr3
    uint64_t fs_base;
    process_t* proc;
    task* next;
    task_state state;
    const void* wait_channel;
    regs* trap_frame;
};

/**
 * Currently active task
 */
extern task* current;

namespace process {
    /**
     * Switches to the next task: updates the TSS kernel-stack pointer and the
     * FS base MSR for the new task.
     * @param next_thread The next task to switch to.
     */
    void switch_to_task(task* next_thread);

    /**
     * Saves the caller's callee-saved registers and current RSP into *prev_rsp_slot
     * (skipped when prev_rsp_slot is nullptr, e.g. when the "next" task is
     * already the running task), then loads next_rsp into RSP and, if
     * next_cr3 differs from the current CR3, switches address spaces.
     * @param prev_rsp_slot Where to store the caller's RSP, or nullptr to skip saving it.
     * @param next_rsp The RSP value to switch to.
     * @param next_cr3 The CR3 value to switch to.
     */
    extern "C" void raw_switch_to_task(void** prev_rsp_slot, void* next_rsp, uint64_t next_cr3);

    void write_kqword(uint64_t cr3, uintptr_t virt_addr, uint64_t value);
    void* build_kernel_frame(uint64_t cr3, uintptr_t kernel_stack_top);
    void* build_fork_frame(uint64_t cr3, uintptr_t trap_frame_addr);

    [[noreturn]] void task_bootstrap();
}

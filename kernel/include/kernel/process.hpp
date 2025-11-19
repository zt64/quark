#pragma once

#include <cstdint>

#include "system.hpp"


enum task_state {
    RUNNING,
    SLEEPING,
    STOPPED,
    ZOMBIE
};

struct task {
    uint32_t tid;
    uint32_t pid;
    void* rsp;
    void* rsp0;
    uint64_t cr3;
    task* next;
    task* parent;
    regs* trap_frame;
    void* entry_point;
    uintptr_t stack_base;
    uint32_t stack_size;
    uint64_t module_addr;
    task_state state;
};

namespace process {
    task* fork();
    void write_kernel_qword(const uint64_t cr3, const uintptr_t virt_addr, const uint64_t value);
    void* build_fork_frame(const uint64_t cr3, const uintptr_t kernel_stack_top);
}

extern task* current;

extern "C" void switch_to_task(task* next_thread);
extern "C" void finish_task_switch(task* next);
task* create_task(uintptr_t module_addr);
extern "C" task* context_switch_prepare(task* next, void* old_rsp);
void schedule();

void terminate_task();

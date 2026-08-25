#pragma once

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
    uint64_t fs_base; // %fs segment base (TLS thread pointer), restored on every task switch
    task* next;
    task* parent;
    regs* trap_frame;
    void* entry_point;
    uintptr_t stack_base;
    uint32_t stack_size;
    uint64_t module_addr;
    task_state state;
    const void* wait_channel;
    uintptr_t phdr_addr;
    uint16_t phentsize;
    uint16_t phnum;
};

namespace process {
    task* fork();
    void write_kernel_qword(const uint64_t cr3, const uintptr_t virt_addr, uint64_t value);
}

extern task* current;

extern "C" void switch_to_task(task* next_thread);
task* create_task(uintptr_t module_addr);
void terminate_task(const task* t);

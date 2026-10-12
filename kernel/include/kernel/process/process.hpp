#pragma once

#include <kernel/fs/file.hpp>
#include "task.hpp"

/**
 * Process structure
 * @note: This structure is used to represent a process in the kernel. It contains information about the process's
 * address space, parent process, entry point, stack, and other relevant data.
 */
struct process_t {
    uint32_t pid;
    uint64_t cr3;
    process_t* parent;
    void* entry_point;
    uintptr_t stack_base;
    uint32_t stack_size;
    uint64_t program_addr;
    char* executable;
    char** argv;
    char** envp;
    uintptr_t phdr_addr;
    uint16_t phentsize;
    uint16_t phnum;
    task* main_task;
    char* cwd;
};

namespace process {
    process_t* create_task(uintptr_t program_addr, char** argv, char** envp);
    process_t* fork_current(task* parent_task);

    /**
    * Replace the calling task's program image in place: same pid, same
    * parent, same tid -- only the address space, entry point and stacks
    * change. This preserves fork()/exec() semantics (a forked child that
    * execs must keep the pid its parent already knows about) instead of
    * spinning up an entirely new, unrelated process.
     * @param t
     * @param program_addr
     * @param path
     * @return
     */
    process_t* exec_current(task* t, uintptr_t program_addr, const char* path, char** argv, char** envp);
}

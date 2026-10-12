#include "kernel/process/process.hpp"

#include <arch/msr.hpp>
#include <kernel/elf.hpp>
#include <kernel/log.hpp>
#include <kernel/process/scheduler.hpp>
#include <kernel/system.hpp>
#include <lib/mem.hpp>
#include <memory/heap.hpp>
#include <lib/string.hpp>
#include <memory/paging.hpp>
#include <memory/vmm.hpp>

namespace process {
    constexpr uint32_t user_stack_pages = 128; // TODO: Dynamically allocate user stack size
    constexpr uint32_t kernel_stack_pages = 128;
    constexpr uintptr_t kernel_stack_size = kernel_stack_pages * paging::PAGE_SIZE;

    /**
     * Process ID counter
     * TODO: Implement PID recycling when processes exit, to avoid PID exhaustion
     */
    uint32_t pid = 1;

    namespace {
        struct task_stacks {
            uintptr_t user_stack_base; // 0 if not requested (kernel-only tasks)
            uintptr_t kernel_stack_base;
        };

        // Allocates the user-mode stack (optional) and kernel-mode stack
        // every task needs, inside the given address space. Shared by
        // create_task/fork_current/exec_current so the sizing/flags/error
        // handling only lives in one place.
        task_stacks allocate_task_stacks(const uint64_t cr3, const bool with_user_stack) {
            task_stacks stacks{};

            if (with_user_stack) {
                stacks.user_stack_base = vmm::alloc(
                    cr3,
                    user_stack_pages * paging::PAGE_SIZE,
                    VM_FLAG_USER | VM_FLAG_WRITE
                );

                if (stacks.user_stack_base == 0) {
                    panic("allocate_task_stacks: unable to allocate user-mode stack");
                }
            }

            stacks.kernel_stack_base = vmm::alloc(
                cr3,
                kernel_stack_size,
                VM_FLAG_WRITE
            );

            if (stacks.kernel_stack_base == 0) {
                panic("allocate_task_stacks: unable to allocate kernel stack");
            }

            return stacks;
        }
    }

    process_t* create_task(const uintptr_t program_addr, char** argv, char** envp) {
        if (program_addr == 0) {
            panic("create_task: program_addr is null");
        }

        const auto t = new task();
        const uint64_t cr3 = paging::new_address_space();
        const auto* img = elf::load_file(cr3, program_addr);

        constexpr uint32_t stack_size = user_stack_pages * paging::PAGE_SIZE;
        const auto [stack_base, kernel_stack_base] = allocate_task_stacks(cr3, true);

        if (pid >= scheduler::PID_MAX) {
            panic("create_task: pid table exhausted");
        }

        auto* proc = new process_t();
        proc->pid = pid;
        proc->cr3 = cr3;
        proc->cwd = strdup("/");
        proc->entry_point = img->entry_point;
        proc->phdr_addr = img->phdr_addr;
        proc->phentsize = img->phentsize;
        proc->phnum = img->phnum;
        proc->stack_base = stack_base;
        proc->stack_size = stack_size;
        proc->program_addr = program_addr;
        proc->main_task = t;
        proc->executable = strdup("unknown"); // TODO: Set to the actual executable path if available
        proc->argv = argv;
        proc->envp = envp;

        t->tid = pid;
        t->cr3 = cr3;
        t->rsp0 = reinterpret_cast<void*>(kernel_stack_base + kernel_stack_size);
        t->rsp = build_kernel_frame(cr3, reinterpret_cast<uintptr_t>(t->rsp0));
        t->proc = proc;
        t->next = t;
        t->state = RUNNING;

        pid++;

        return proc;
    }

    process_t* fork_current(task* parent_task) {
        if (parent_task == nullptr) {
            panic("fork_current: parent_task is null");
        }

        process_t* parent_proc = parent_task->proc;

        const uint64_t child_cr3 = paging::new_address_space();

        auto* child_proc = new process_t();
        child_proc->pid = pid++;
        child_proc->cr3 = child_cr3;
        child_proc->parent = parent_proc;
        child_proc->entry_point = parent_proc->entry_point;
        child_proc->phdr_addr = parent_proc->phdr_addr;
        child_proc->phentsize = parent_proc->phentsize;
        child_proc->phnum = parent_proc->phnum;
        child_proc->stack_base = parent_proc->stack_base;
        child_proc->stack_size = parent_proc->stack_size;
        child_proc->program_addr = parent_proc->program_addr;
        child_proc->executable = strdup(parent_proc->executable ? parent_proc->executable : "");
        child_proc->cwd = strdup(parent_proc->cwd ? parent_proc->cwd : "/");

        vmm::clone_address_space(child_cr3, parent_proc->cr3);

        // Fork reuses the parent's user-mode stack mapping (cloned above),
        // so only a fresh kernel stack is needed here.
        const auto [_, child_kernel_base] = allocate_task_stacks(child_cr3, false);

        auto* child_task = new task();
        child_task->tid = pid++;
        child_task->proc = child_proc;
        child_task->cr3 = child_cr3;
        child_task->rsp0 = reinterpret_cast<void*>(child_kernel_base + kernel_stack_size);
        child_task->fs_base = parent_task->fs_base;
        child_task->state = RUNNING;
        child_proc->main_task = child_task;

        const uintptr_t parent_kernel_base = reinterpret_cast<uintptr_t>(parent_task->rsp0) - kernel_stack_size;

        for (uintptr_t offset = 0; offset < kernel_stack_size; offset += paging::PAGE_SIZE) {
            uintptr_t parent_phys;
            if (!paging::translate(parent_proc->cr3, parent_kernel_base + offset, parent_phys)) {
                panic("fork: failed to translate parent kernel stack page");
            }
            uintptr_t child_phys;
            if (!paging::translate(child_cr3, child_kernel_base + offset, child_phys)) {
                panic("fork: failed to translate child kernel stack page");
            }
            memcpy(
                reinterpret_cast<void*>(child_phys + paging::g_hhdm_offset),
                reinterpret_cast<const void*>(parent_phys + paging::g_hhdm_offset),
                paging::PAGE_SIZE
            );
        }

        const uintptr_t trap_offset =
            reinterpret_cast<uintptr_t>(parent_task->trap_frame) - parent_kernel_base;
        child_task->trap_frame = reinterpret_cast<regs*>(child_kernel_base + trap_offset);

        child_task->rsp = build_fork_frame(
            child_cr3,
            reinterpret_cast<uintptr_t>(child_task->trap_frame)
        );

        write_kqword(
            child_cr3,
            reinterpret_cast<uintptr_t>(child_task->trap_frame) + offsetof(regs, rax),
            0
        );

        scheduler::register_process(child_proc);
        scheduler::schedule_task(child_task);

        return child_proc;
    }

    static char** copy_user_strv(char** src) {
        if (!src) {
            auto* copy = static_cast<char**>(kalloc(sizeof(char*)));
            copy[0] = nullptr;
            return copy;
        }

        size_t count = 0;
        while (src[count] != nullptr) count++;

        auto* copy = static_cast<char**>(kalloc((count + 1) * sizeof(char*)));
        if (!copy) return nullptr;

        for (size_t i = 0; i < count; i++) {
            copy[i] = strdup(src[i]);
        }
        copy[count] = nullptr;

        return copy;
    }


    process_t* exec_current(task* t, const uintptr_t program_addr, const char* path, char** argv, char** envp) {
        process_t* proc = t->proc;

        const uint64_t new_cr3 = paging::new_address_space();
        const auto* img = elf::load_file(new_cr3, program_addr);
        if (img == nullptr) {
            logger.error("exec_current: failed to load ELF image for %s", path);
            paging::free_address_space(new_cr3);
            return nullptr;
        }

        constexpr uint32_t stack_size = user_stack_pages * paging::PAGE_SIZE;
        const auto [stack_base, kernel_stack_base] = allocate_task_stacks(new_cr3, true);

        const uint64_t old_cr3 = proc->cr3;
        const uintptr_t old_prog_addr = proc->program_addr;

        proc->cr3 = new_cr3;
        proc->entry_point = img->entry_point;
        proc->phdr_addr = img->phdr_addr;
        proc->phentsize = img->phentsize;
        proc->phnum = img->phnum;
        proc->stack_base = stack_base;
        proc->stack_size = stack_size;
        proc->program_addr = program_addr;
        kfree(proc->executable);
        proc->executable = strdup(path);
        proc->argv = copy_user_strv(argv);
        proc->envp = copy_user_strv(envp);

        // Replace the calling task's context in place -- same tid.
        t->cr3 = new_cr3;
        t->rsp0 = reinterpret_cast<void*>(kernel_stack_base + kernel_stack_size);
        t->rsp = build_kernel_frame(new_cr3, reinterpret_cast<uintptr_t>(t->rsp0));
        t->trap_frame = nullptr;
        t->state = RUNNING;

        // TODO: free every mapped page under old_cr3 (vm_objects, ELF
        // segments), not just the top-level PML4 page. Same "reaper not
        // implemented yet" caveat as scheduler::terminate_proc().
        paging::free_address_space(old_cr3);
        kfree(reinterpret_cast<void*>(old_prog_addr));

        return proc;
    }
}

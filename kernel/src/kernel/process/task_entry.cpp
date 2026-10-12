#include "kernel/process/userspace.hpp"

#include <cstddef>
#include <cstdlib>
#include <driver/timer.hpp>
#include <kernel/log.hpp>
#include <kernel/process/process.hpp>
#include <kernel/system.hpp>
#include <arch/x86_64/gdt.hpp>
#include <arch/x86_64/tss.hpp>
#include <lib/mem.hpp>
#include <lib/string.hpp>
#include <memory/heap.hpp>
#include <memory/paging.hpp>

// Task entry: getting a runnable task/process pair from "scheduled" to
// actually executing in ring 3. Builds the SysV-ABI-compliant initial user
// stack (argv/envp/auxv) and performs the ring0->ring3 transition. See
// userspace.cpp for how the program image itself gets loaded beforehand.

namespace userspace {
    namespace {
        [[noreturn]]
        void switch_to_userspace(uint64_t entry, uint64_t user_stack) {
            constexpr uint64_t kUserFlags = 0x202;
            asm volatile(
                "pushq %[ds]\n"
                "pushq %[stack]\n"
                "pushq %[flags]\n"
                "pushq %[cs]\n"
                "pushq %[entry]\n"
                "iretq\n"
                :
                : [ds] "r"(static_cast<uint64_t>(gdt::USER_DS)),
                [stack] "r"(user_stack),
                [flags] "r"(kUserFlags),
                [cs] "r"(static_cast<uint64_t>(gdt::USER_CS)),
                [entry] "r"(entry)
                : "memory");

            __builtin_unreachable();
        }

        uint32_t write_user_bytes(const uint64_t cr3, const uintptr_t virt, const void* data, const size_t len) {
            const auto* src = static_cast<const uint8_t*>(data);
            size_t written = 0;

            while (written < len) {
                const uintptr_t addr = virt + written;
                uintptr_t phys;

                if (!paging::translate(cr3, addr, phys)) {
                    panic("write_user_bytes: unmapped user address 0x%lx", addr);
                }

                const size_t page_off = addr & (paging::PAGE_SIZE - 1);
                size_t chunk = paging::PAGE_SIZE - page_off;
                if (chunk > len - written) chunk = len - written;
                memcpy(reinterpret_cast<void*>(phys + paging::g_hhdm_offset), src + written, chunk);
                written += chunk;
            }

            return written;
        }

        uintptr_t build_initial_stack(const uint64_t cr3, const uintptr_t stack_top, const task* t,
                                      const char* path, char** argv, char** envp) {
            constexpr uint64_t AT_NULL = 0, AT_PHDR = 3, AT_PHENT = 4, AT_PHNUM = 5,
                               AT_PAGESZ = 6, AT_BASE = 7, AT_ENTRY = 9;

            uintptr_t sp = stack_top;

            const size_t path_len = strlen(path) + 1;
            sp -= path_len;
            size_t argc = 0;

            while (argv && argv[argc] != nullptr) {
                argc++;
            }

            const auto args = static_cast<char**>(kalloc((argc + 1) * sizeof(char*)));
            args[0] = const_cast<char*>(path);
            for (size_t i = 1; i < argc; i++) {
                args[i] = strdup(argv[i]);
            }
            args[argc] = nullptr;

            const auto argv_ptrs = static_cast<uint64_t*>(kalloc((argc + 1) * sizeof(uint64_t)));

            for (int i = argc - 1; i >= 0; --i) {
                const size_t len = strlen(args[i]) + 1;

                sp -= len;
                write_user_bytes(cr3, sp, args[i], len);

                argv_ptrs[i] = sp;
            }

            argv_ptrs[argc] = 0;

            const process_t* proc = t->proc;

            struct auxv_entry {
                uint64_t type;
                uint64_t value;
            };

            const auxv_entry auxv[] = {
                {.type = AT_PHDR, .value = proc->phdr_addr},
                {.type = AT_PHENT, .value = proc->phentsize},
                {.type = AT_PHNUM, .value = proc->phnum},
                {.type = AT_PAGESZ, .value = paging::PAGE_SIZE},
                {.type = AT_BASE, .value = 0},
                {.type = AT_ENTRY, .value = reinterpret_cast<uint64_t>(proc->entry_point)},
                {.type = AT_NULL, .value = 0},
            };

            constexpr uint64_t envp_ptrs[1] = {};

            const size_t argv_ptrs_size = (argc + 1) * sizeof(uint64_t);

            const size_t total = sizeof(argc) + argv_ptrs_size + sizeof(envp_ptrs) + sizeof(auxv);
            sp -= total;
            sp &= ~static_cast<uintptr_t>(0xF); // 16 byte aligned

            uintptr_t p = sp;

            p += write_user_bytes(cr3, p, &argc, sizeof(argc));
            p += write_user_bytes(cr3, p, argv_ptrs, argv_ptrs_size);
            p += write_user_bytes(cr3, p, envp_ptrs, sizeof(envp_ptrs));
            write_user_bytes(cr3, p, auxv, sizeof(auxv));

            return sp;
        }
    }

    [[noreturn]] void launch(const char* path, char** argv, char** envp,
                             const char* const env_vars[]) {
        const process_t* p = spawn(path, argv, envp, env_vars);
        if (p == nullptr) {
            panic("launch: failed to spawn %s", path);
        }
        process::switch_to_task(p->main_task);
        __builtin_unreachable();
    }

    [[noreturn]]
    void launch_init() {
        logger.info("Launching init");
        launch("/BIN/INIT", nullptr, nullptr, nullptr);
        __builtin_unreachable();
    }

    [[noreturn]]
    void enter_task(task* task) {
        current = task;
        const process_t* proc = task->proc;
        paging::switch_cr3(proc->cr3);
        tss::set_kernel_stack(reinterpret_cast<uintptr_t>(task->rsp0));

        asm volatile(
            "mov %0, %%rsp"
            :
            : "r"(task->rsp0)
            : "memory"
        );

        const uintptr_t initial_sp = build_initial_stack(proc->cr3, proc->stack_base + proc->stack_size,
                                                         task, proc->executable, proc->argv, proc->envp);
        switch_to_userspace(reinterpret_cast<uintptr_t>(proc->entry_point), initial_sp);
        __builtin_unreachable();
    }
}

#include "kernel/userspace.hpp"
#include <cstddef>
#include <driver/fs/vfs.hpp>
#include <kernel/scheduler.hpp>

#include "boot/limine/limine_requests.hpp"
#include "kernel/log.hpp"
#include "kernel/process.hpp"
#include "kernel/system.hpp"
#include "kernel/tss.hpp"
#include "lib/mem.hpp"
#include "lib/stdlib.hpp"
#include "lib/string.hpp"
#include "memory/paging.hpp"

extern "C" void enter_usermode(uintptr_t entry, uintptr_t stack_top);

namespace userspace {
    namespace {
        constexpr uint8_t USER_DS = 0x23; // GDT user data segment
        constexpr uint8_t USER_CS = 0x1B; // GDT user code segment

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
                : [ds] "r"(static_cast<uint64_t>(USER_DS)),
                [stack] "r"(user_stack),
                [flags] "r"(kUserFlags),
                [cs] "r"(static_cast<uint64_t>(USER_CS)),
                [entry] "r"(entry)
                : "memory");

            __builtin_unreachable();
        }

        namespace {
            void write_user_bytes(const uint64_t cr3, const uintptr_t virt, const void* data, const size_t len) {
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
            }

            uintptr_t build_initial_stack(const uint64_t cr3, const uintptr_t stack_top, const task* t,
                                          const char* path) {
                constexpr uint64_t AT_NULL = 0, AT_PHDR = 3, AT_PHENT = 4, AT_PHNUM = 5,
                                   AT_PAGESZ = 6, AT_BASE = 7, AT_ENTRY = 9;

                uintptr_t sp = stack_top;

                const size_t path_len = strlen(path) + 1;
                sp -= path_len;
                const uintptr_t argv0_addr = sp;
                write_user_bytes(cr3, sp, path, path_len);

                const struct {
                    uint64_t type;
                    uint64_t value;
                } auxv[] = {
                    {.type = AT_PHDR, .value = t->phdr_addr},
                    {.type = AT_PHENT, .value = t->phentsize},
                    {.type = AT_PHNUM, .value = t->phnum},
                    {.type = AT_PAGESZ, .value = paging::PAGE_SIZE},
                    {.type = AT_BASE, .value = 0},
                    {.type = AT_ENTRY, .value = reinterpret_cast<uint64_t>(t->entry_point)},
                    {.type = AT_NULL, .value = 0},
                };

                const uint64_t argv_ptrs[2] = {argv0_addr, 0};
                constexpr uint64_t envp_ptrs[1] = {};
                constexpr uint64_t argc = 1;

                constexpr size_t total = sizeof(argc) + sizeof(argv_ptrs) + sizeof(envp_ptrs) + sizeof(auxv);
                sp -= total;
                sp &= ~static_cast<uintptr_t>(0xF); // SysV ABI: SP must be 16-byte aligned at entry

                uintptr_t p = sp;
                write_user_bytes(cr3, p, &argc, sizeof(argc));
                p += sizeof(argc);
                write_user_bytes(cr3, p, argv_ptrs, sizeof(argv_ptrs));
                p += sizeof(argv_ptrs);
                write_user_bytes(cr3, p, envp_ptrs, sizeof(envp_ptrs));
                p += sizeof(envp_ptrs);
                write_user_bytes(cr3, p, auxv, sizeof(auxv));

                return sp;
            }
        }
    }

    void launch(const char* path, const char* const argv[], char* const envp[], const char* const env_vars[]) {
        (void)argv;
        (void)envp;
        (void)env_vars;

        vfs::stat_t stat{};
        if (!vfs::stat(path, &stat)) {
            panic("Failed to stat %s", path);
        }

        const uint32_t max_buffer_size = stat.size;
        const auto program_buf = static_cast<uint8_t*>(malloc(max_buffer_size));

        if (!program_buf) {
            panic("Failed to allocate buffer");
        }

        if (!vfs::read(path, program_buf, max_buffer_size)) {
            panic("Failed to read %s", path);
        }

        task* t = create_task(reinterpret_cast<uintptr_t>(program_buf));

        if (current == nullptr) {
            current = t;
            enter_task(t);
        }

        switch_to_task(t);
    }

    [[noreturn]]
    void launch_init() {
        logger.info("Launching init");
        launch("/BOOT/INIT", nullptr, nullptr, nullptr);
        __builtin_unreachable();
    }

    void enter_task(task* task) {
        current = task;
        paging::switch_cr3(task->cr3);
        tss::set_kernel_stack(reinterpret_cast<uintptr_t>(task->rsp0));

        const uintptr_t initial_sp = build_initial_stack(task->cr3, task->stack_base + task->stack_size,
                                                         task,
                                                         "/BOOT/INIT");
        switch_to_userspace(reinterpret_cast<uintptr_t>(task->entry_point), initial_sp);
        // switch_to_userspace(reinterpret_cast<uintptr_t>(current->entry_point), current->stack_base + current->stack_size);
        __builtin_unreachable();
    }
}

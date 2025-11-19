#include "kernel/userspace.hpp"
#include <cstddef>
#include <driver/fs/vfs.hpp>
#include <kernel/process.hpp>
#include <lib/stdlib.hpp>
#include <memory/pmm.hpp>
#include "lib/string.hpp"

#include "boot/limine/limine_requests.hpp"
#include "kernel/elf.hpp"
#include "kernel/log.hpp"
#include "kernel/system.hpp"
#include "kernel/tss.hpp"
#include "memory/paging.hpp"
#include "memory/vmm.hpp"

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

        uintptr_t get_module_address(const char* const module_name) {
            const limine_module_response* module_response = limine_requests::module_request.response;
            if (!module_response || module_response->module_count == 0) {
                panic("No modules provided");
            }

            const limine_file* module = nullptr;

            for (size_t i = 0; i < module_response->module_count; ++i) {
                if (strcmp(module_response->modules[i]->path, module_name) == 0) {
                    module = module_response->modules[i];
                    break;
                }
            }

            if (!module) {
                panic("Module not found");
            }

            return reinterpret_cast<uintptr_t>(module->address);
        }
    }

    void launch(const char* path, const char* const argv[], char* const envp[], const char* const env_vars[]) {
        (void)argv;
        (void)envp;
        (void)env_vars;

        constexpr uint32_t max_buffer_size = 512 * 512; // 256 KiB
        const auto init_buffer = static_cast<uint8_t*>(malloc(max_buffer_size));

        if (!init_buffer) {
            panic("Failed to allocate buffer");
        }
        if (!vfs::read(path, init_buffer, max_buffer_size)) {
            panic("Failed to read %s", path);
        }

        const auto snell_buffer = static_cast<uint8_t*>(malloc(max_buffer_size));

        if (!snell_buffer) {
            panic("Failed to allocate buffer");
        }
        if (!vfs::read("/BOOT/SNELL", snell_buffer, max_buffer_size)) {
            panic("Failed to read /BOOT/SNELL");
        }

        current = create_task(reinterpret_cast<uintptr_t>(init_buffer));
        task* next = create_task(reinterpret_cast<uintptr_t>(snell_buffer));

        current->next = next;
        next->next = current;

        enter_task(current);
        __builtin_unreachable();
    }

    [[noreturn]]
    void launch_init() {
        logger.info("Launching init");
        launch("/BOOT/INIT", nullptr, nullptr, nullptr);
    }

    void enter_task(task* task) {
        current = task;
        paging::switch_cr3(current->cr3);
        tss::set_kernel_stack(reinterpret_cast<uintptr_t>(current->rsp0));

        switch_to_userspace(reinterpret_cast<uintptr_t>(current->entry_point), current->stack_base + current->stack_size);
        __builtin_unreachable();
    }
}

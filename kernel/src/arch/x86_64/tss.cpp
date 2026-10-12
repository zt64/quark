#include "arch/x86_64/tss.hpp"
#include <cstdint>
#include "arch/x86_64/gdt.hpp"
#include "lib/mem.hpp"

namespace tss {
    namespace {
        alignas(16) uint8_t kernel_stack[16 * 1024];
    }

    extern "C" tss_entry_t tss_entry = {};

    void init() {
        memset(&tss_entry, 0, sizeof(tss_entry));

        tss_entry.rsp0 = reinterpret_cast<uintptr_t>(kernel_stack) + sizeof(kernel_stack);
        tss_entry.iomap_base = sizeof(tss_entry);

        gdt::install_tss(
            reinterpret_cast<uintptr_t>(&tss_entry),
            sizeof(tss_entry) - 1
        );

        asm volatile(
            "movw $0x28, %%ax\n"
            "ltr %%ax\n"
            : : : "ax"
        );
    }

    void set_kernel_stack(const uintptr_t rsp0) {
        tss_entry.rsp0 = rsp0;
    }
}

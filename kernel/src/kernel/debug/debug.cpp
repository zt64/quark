#include <cstdint>
#include <kernel/log.hpp>
#include <kernel/debug/debug.hpp>

namespace debug {
    static uintptr_t get_rbp() {
        uintptr_t rbp;
        asm volatile(
            "mov %%rbp, %0"
            : "=r"(rbp)
        );
        return rbp;
    }

    void print_stacktrace(const size_t max_frames) {
        auto* rbp = reinterpret_cast<uintptr_t*>(get_rbp());

        logger.error("Stack trace:");

        for (size_t i = 0; i < max_frames && rbp; ++i) {
            // x86-64 stack frame:
            //
            // rbp + 0 = previous RBP
            // rbp + 8 = return address

            const uintptr_t previous_rbp = rbp[0];
            const uintptr_t return_addr = rbp[1];

            if (return_addr == 0) break;

            logger.error(
                "  #%zu: 0x%lx",
                i,
                return_addr
            );

            // Basic sanity checks to avoid following garbage forever.
            if (previous_rbp <= reinterpret_cast<uintptr_t>(rbp))
                break;

            if (previous_rbp & 0x7)
                break;

            rbp = reinterpret_cast<uintptr_t*>(previous_rbp);
        }
    }
}

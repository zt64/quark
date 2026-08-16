#include "kernel/platform.hpp"

#include <arch/msr.hpp>

#include "driver/pic.hpp"
#include "kernel/gdt.hpp"
#include "kernel/idt.hpp"
#include "kernel/log.hpp"
#include "kernel/tss.hpp"

namespace platform {
    void init_fpu() {
        // Limine only guarantees PG/PE/PAE/WP/LME/NX are set on entry; it does
        // not enable the FPU or SSE. Without CR4.OSFXSR/OSXMMEXCPT set, any
        // SSE instruction the compiler emits (e.g. for float/double math)
        // raises #UD. This must run before any floating-point code executes,
        // including anything reachable from boot::init_early().

        // TODO: Check CPUID for support

        uint64_t cr0;
        asm volatile("mov %%cr0, %0" : "=r"(cr0));
        cr0 &= ~static_cast<uint64_t>(1 << 2); // clear EM (no x87/SSE emulation)
        cr0 |= static_cast<uint64_t>(1 << 1); // set MP (monitor co-processor)
        asm volatile("mov %0, %%cr0" : : "r"(cr0));

        uint64_t cr4;
        asm volatile("mov %%cr4, %0" : "=r"(cr4));
        cr4 |= static_cast<uint64_t>(1 << 9) | static_cast<uint64_t>(1 << 10); // OSFXSR, OSXMMEXCPT
        asm volatile("mov %0, %%cr4" : : "r"(cr4));

        asm volatile("fninit");
    }

    void init() {
        gdt::init();
        tss::init();
        logger.info("GDT initialized");

        idt::init();
        logger.info("IDT initialized");

        pic::init();
        logger.info("IRQ initialized");
    }

    void enable_interrupts() {
        asm volatile("sti");
        logger.info("Interrupts enabled");
    }
}

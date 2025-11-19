#include "kernel/platform.hpp"

#include "driver/pic.hpp"
#include "kernel/gdt.hpp"
#include "kernel/idt.hpp"
#include "kernel/log.hpp"
#include "kernel/tss.hpp"

namespace platform {
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

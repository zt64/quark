#include "arch/x86_64/idt.hpp"
#include <arch/x86_64/gdt.hpp>
#include "arch/isr.hpp"
#include "lib/mem.hpp"

namespace idt {
    __attribute__((aligned(0x10))) // TODO: Is this actually needed
    idt_entry_t idt_table[256];
    idtr_t idtr;

    void init() {
        // Sets the special IDT pointer up
        idtr.limit = (sizeof(idt_entry_t) * 256) - 1;
        idtr.base = reinterpret_cast<uintptr_t>(&idt_table);

        /* Clear out the entire IDT, initializing it to zeros */
        memset(&idt_table, 0, sizeof(idt_entry_t) * 256);

        // Load CPU exception handlers 0 - 32
        isr::init();

        // Call "lidt" instruction to tell CPU where IDT is located
        idt_load();
    }

    void set_gate(const uint8_t num, const uint64_t base, const uint8_t flags) {
        idt_entry_t* descriptor = &idt_table[num];

        descriptor->isr_low = base & 0xFFFF; // lower 16 bits of the ISR address
        descriptor->kernel_cs = gdt::selector::KERNEL_CS;
        descriptor->ist = 0; // no IST used
        descriptor->isr_high = (base >> 16) & 0xFFFF; // bits 16-31 of the ISR address
        descriptor->isr_high2 = (base >> 32) & 0xFFFFFFFF; // bits 32-63 of the ISR address
        descriptor->attributes = flags;
        descriptor->reserved = 0;
    }
}

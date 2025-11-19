#include "kernel/idt.hpp"
#include "arch/isr.hpp"
#include "driver/fb.hpp"
#include "lib/mem.hpp"

__attribute__((aligned(0x10)))
idt_entry_t idt_table[256];
idtr_t idtr;

namespace idt {
    void set_gate(const uint8_t num, const uint64_t base, const uint8_t flags) {
        idt_entry_t* descriptor = &idt_table[num];

        descriptor->isr_low = base & 0xFFFF;
        descriptor->kernel_cs = 0x08;
        descriptor->ist = 0;
        descriptor->isr_high = (base >> 16) & 0xFFFF;
        descriptor->isr_high2 = (base >> 32) & 0xFFFFFFFF;
        descriptor->attributes = flags;
        descriptor->reserved = 0;
    }

    void init() {
        // Sets the special IDT pointer up
        idtr.limit = (sizeof(idt_entry_t) * 256) - 1;
        idtr.base = reinterpret_cast<uintptr_t>(&idt_table);

        /* Clear out the entire IDT, initializing it to zeros */
        memset(&idt_table, 0, sizeof(idt_entry_t) * 256);

        // Load CPU exception handlers 0 - 32
        isrs_init();

        // Call "lidt" instruction to tell CPU where IDT is located
        idt_load();
    }
}
#pragma once

#include <cstdint>

namespace idt {
    void set_gate(uint8_t num, uint64_t base, uint8_t flags);

    void init();
}

typedef struct {
    uint16_t isr_low; // The lower 16 bits of the ISR's address
    uint16_t kernel_cs; // The GDT segment selector that the CPU will load into CS before calling the ISR
    uint8_t ist;
    uint8_t attributes; // Type and attributes; see the IDT page
    uint16_t isr_high; // bits 16..31
    uint32_t isr_high2; // bits 32..63
    uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idtr_t;

extern "C" void idt_load();
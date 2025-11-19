#include "arch/isr.hpp"

#include <driver/pic.hpp>
#include <kernel/process.hpp>

#include "kernel/exception.hpp"
#include "kernel/idt.hpp"
#include "kernel/system.hpp"

extern "C" void* isr_stub_table[];

extern "C" void isr_stub_128();

void isrs_init() {
    for (uint32_t i = 0; i < 32; i++) {
        idt::set_gate(i, reinterpret_cast<uint64_t>(isr_stub_table[i]), 0x8E);
    }
    idt::set_gate(0x80, reinterpret_cast<uint64_t>(isr_stub_128), 0xEE);
}

extern "C" void irq_handle(const regs* r);        // from pic.cpp
extern "C" void handle_syscall(regs* r); // from syscall.cpp

extern "C" regs* isr_dispatch(regs* r) {
    if (current) {
        current->trap_frame = r;
    }

    if (r->int_no == 0x80) {
        handle_syscall(r);
        return r;
    }

    if (r->int_no >= 32 && r->int_no < 48) {
        irq_handle(r);
        return r;
    }

    fault_handler(r);
}
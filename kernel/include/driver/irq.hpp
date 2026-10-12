#pragma once

#include "kernel/system.hpp"

typedef void (*irq_handler)(const regs* r);

namespace irq {
    extern "C" void* irq_stub_table[];

    /**
     * Install an interrupt handler for the given IRQ number.
     * @param irq The IRQ number for which the handler is to be installed.
     * @param handler The function pointer to the handler that will be called when the interrupt occurs.
     */
    void install_handler(uint8_t irq, irq_handler handler);

    /**
     * Uninstall the interrupt handler for the given IRQ number.
     * @param irq The IRQ number for which the handler is to be uninstalled.
     */
    void uninstall_handler(uint8_t irq);
}

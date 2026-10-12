#pragma once

/**
 * Programmable Interrupt Controller
 */
namespace pic {
    void init();

    /**
     * Mask the given IRQ line, preventing it from generating interrupts
     * @param irq The IRQ number to be masked
     */
    void mask_irq(uint8_t irq);

    /**
     * Unmask the given IRQ line, allowing it to generate interrupts
     * @param irq The IRQ number to be unmasked
     */
    void unmask_irq(uint8_t irq);

    /**
     * Disable the PIC, preventing it from generating any interrupts
     */
    void disable();

    /**
     * Remap the PIC to avoid conflicts with CPU exceptions
     */
    void remap();

    /**
     * Send End of Interrupt (EOI) signal to the PIC for the given IRQ
     * @param irq The IRQ number that has been handled and for which the EOI signal should be sent
     */
    void send_eoi(uint8_t irq);
}

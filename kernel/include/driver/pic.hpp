#pragma once

#include "kernel/system.hpp"

constexpr uint8_t PIC1 = 0x20; /* IO base address for master PIC */
constexpr uint8_t PIC2 = 0xA0; /* IO base address for slave PIC */
constexpr uint8_t PIC1_COMMAND = PIC1;
constexpr uint8_t PIC1_DATA = PIC1 + 1;
constexpr uint8_t PIC2_COMMAND = PIC2;
constexpr uint8_t PIC2_DATA = PIC2 + 1;
constexpr uint8_t PIC_EOI = 0x20;

constexpr uint8_t ICW1_ICW4 = 0x01; /* Indicates that ICW4 will be present */
constexpr uint8_t ICW1_SINGLE = 0x02; /* Single (cascade) mode */
constexpr uint8_t ICW1_INTERVAL4 = 0x04; /* Call address interval 4 (8) */
constexpr uint8_t ICW1_LEVEL = 0x08; /* Level triggered (edge) mode */
constexpr uint8_t ICW1_INIT = 0x10; /* Initialization - required! */

constexpr uint8_t ICW4_8086 = 0x01; /* 8086/88 (MCS-80/85) mode */
constexpr uint8_t ICW4_AUTO = 0x02; /* Auto (normal) EOI */
constexpr uint8_t ICW4_BUF_SLAVE = 0x08; /* Buffered mode/slave */
constexpr uint8_t ICW4_BUF_MASTER = 0x0C; /* Buffered mode/master */
constexpr uint8_t ICW4_SFNM = 0x10; /* Special fully nested (not) */

constexpr uint8_t CASCADE_IRQ = 2;

typedef void (*irq_handler)(const regs* r);

namespace irq {
    void install_handler(uint32_t irq, irq_handler handler);
    void uninstall_handler(uint32_t irq);
}

/**
 * Programmable Interrupt Controller
 */
namespace pic {
    void init();
    void mask_irq(uint8_t irq);
    void unmask_irq(uint8_t irq);
    void disable();
    void remap();
    void send_eoi(uint8_t irq);
}

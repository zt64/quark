#include "driver/pic.hpp"

#include "kernel/idt.hpp"
#include "kernel/system.hpp"

namespace irq {
    static inline irq_handler irq_routines[16];
    extern "C" void* irq_stub_table[];

    extern "C" void irq_handle(const regs* r) {
        if (r->int_no == 255) {
            // Spurious interrupt from APIC
            return;
        }

        if (r->int_no >= 32 && r->int_no < 48) {
            if (const irq_handler handler = irq_routines[r->int_no - 32]) {
                handler(r);
            }
        }

        outb(PIC1_COMMAND, PIC_EOI); // send end of interrupt (EOI) signal
    }

    void install_handler(const uint32_t irq, const irq_handler handler) {
        irq_routines[irq] = handler;
    }

    void uninstall_handler(const uint32_t irq) {
        irq_routines[irq] = nullptr;
    }
}

namespace pic {
    void init() {
        remap();

        for (uint8_t i = 0; i < 16; i++) {
            // Hardware interrupts start at vector 32
            idt::set_gate(32 + i, reinterpret_cast<uint64_t>(irq::irq_stub_table[i]), 0x8E);
        }

        idt::set_gate(255, reinterpret_cast<uint64_t>(irq::irq_stub_table[15]), 0x8E);
    }

    void unmask_irq(uint8_t irq) {
        uint16_t port;

        if (irq < 8) {
            port = PIC1_DATA;
        } else {
            port = PIC2_DATA;
            irq -= 8;
        }

        outb(port, inb(port) & ~(1 << irq));
    }

    void mask_irq(uint8_t irq) {
        uint16_t port;

        if (irq < 8) {
            port = PIC1_DATA;
        } else {
            port = PIC2_DATA;
            irq -= 8;
        }

        outb(port, inb(port) | (1 << irq));
    }

    static void io_wait() {
        outb(0x80, 0);
    }

    void remap() {
        uint8_t mask1 = inb(PIC1_DATA);
        const uint8_t mask2 = inb(PIC2_DATA);

        outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
        io_wait();
        outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
        io_wait();
        outb(PIC1_DATA, 0x20); // ICW2: Master PIC vector offset
        io_wait();
        outb(PIC2_DATA, 0x28); // ICW2: Slave PIC vector offset
        io_wait();
        outb(PIC1_DATA, 1 << CASCADE_IRQ); // ICW3: tell Master PIC that there is a slave PIC at IRQ2
        io_wait();
        outb(PIC2_DATA, CASCADE_IRQ); // ICW3: tell Slave PIC its cascade identity (0000 0010)
        io_wait();

        outb(PIC1_DATA, ICW4_8086); // ICW4: have the PICs use 8086 mode (and not 8080 mode)
        io_wait();
        outb(PIC2_DATA, ICW4_8086);
        io_wait();

        mask1 &= ~(1 << 0);

        outb(PIC1_DATA, mask1);
        io_wait();
        outb(PIC2_DATA, mask2);
        io_wait();
    }

    void disable() {
        outb(PIC1_DATA, 0xFF);
        outb(PIC2_DATA, 0xFF);
    }

    void send_eoi(uint8_t irq) {
        if (irq >= 8) {
            outb(PIC2_COMMAND, PIC_EOI);
        }
        outb(PIC1_COMMAND, PIC_EOI);
    }
}

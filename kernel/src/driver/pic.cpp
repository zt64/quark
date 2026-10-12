#include <arch/x86_64/idt.hpp>
#include <driver/irq.hpp>
#include <driver/pic.hpp>
#include <driver/timer.hpp>
#include <kernel/system.hpp>

namespace pic {
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

    constexpr uint8_t IDT_INTERRUPT_GATE_FLAGS = 0x8E; /* present, ring 0, 32-bit interrupt gate */
    constexpr uint8_t PIC1_VECTOR_OFFSET = 0x20; /* ICW2: master PIC vector offset */
    constexpr uint8_t PIC2_VECTOR_OFFSET = 0x28; /* ICW2: slave PIC vector offset */
    constexpr uint16_t IO_DELAY_PORT = 0x80; /* unused POST diagnostic port, used for I/O delay */
    constexpr uint8_t PIC_MASK_ALL = 0xFF; /* mask value that disables all 8 IRQ lines */

    constexpr uint8_t IRQ_COUNT = 16;
    constexpr uint8_t IRQ_BASE_VECTOR = 32;
    constexpr uint8_t SPURIOUS_VECTOR = 255;

    void init() {
        remap();

        for (uint8_t i = 0; i < IRQ_COUNT; i++) {
            // Hardware interrupts start at vector 32
            idt::set_gate(IRQ_BASE_VECTOR + i, reinterpret_cast<uint64_t>(irq::irq_stub_table[i]),
                          IDT_INTERRUPT_GATE_FLAGS);
        }

        idt::set_gate(SPURIOUS_VECTOR, reinterpret_cast<uint64_t>(irq::irq_stub_table[15]), IDT_INTERRUPT_GATE_FLAGS);

        timer::init(1000);
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
        outb(IO_DELAY_PORT, 0);
    }

    void send_cmd(const uint8_t cmd, const uint8_t pic) {
        if (pic == 1) {
            outb(PIC1_COMMAND, cmd);
        } else if (pic == 2) {
            outb(PIC2_COMMAND, cmd);
        }
    }

    void remap() {
        uint8_t mask1 = inb(PIC1_DATA);
        const uint8_t mask2 = inb(PIC2_DATA);

        outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
        io_wait();
        outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
        io_wait();
        outb(PIC1_DATA, PIC1_VECTOR_OFFSET); // ICW2: Master PIC vector offset
        io_wait();
        outb(PIC2_DATA, PIC2_VECTOR_OFFSET); // ICW2: Slave PIC vector offset
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
        outb(PIC1_DATA, PIC_MASK_ALL);
        outb(PIC2_DATA, PIC_MASK_ALL);
    }

    void send_eoi(const uint8_t irq) {
        if (irq >= 8) {
            outb(PIC2_COMMAND, PIC_EOI);
        }
        outb(PIC1_COMMAND, PIC_EOI);
    }
}

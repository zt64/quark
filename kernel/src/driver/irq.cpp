#include "driver/irq.hpp"

#include <kernel/log.hpp>

#include "driver/pic.hpp"
#include "kernel/system.hpp"

namespace irq {
    constexpr uint8_t IRQ_ROUTINES_SIZE = 16;

    static inline irq_handler irq_routines[IRQ_ROUTINES_SIZE];

    extern "C" void irq_handle(const regs* r) {
        if (r == nullptr) {
            panic("irq_handle: null trap frame");
        }

        const uint64_t vector = r->int_no;

        // Spurious interrupt from APIC
        if (vector == 255) {
            return;
        }

        if (vector < 32 || vector >= 48) {
            logger.error(
                "irq_handle: invalid IRQ vector %lu",
                vector
            );
            return;
        }

        const uint8_t irq = static_cast<uint8_t>(vector - 32);

        if (const irq_handler handler = irq_routines[irq]) {
            handler(r);
        } else {
            logger.warn("Unhandled IRQ %u", irq);
        }

        pic::send_eoi(irq);
    }

    void install_handler(const uint8_t irq, const irq_handler handler) {
        irq_routines[irq] = handler;
    }

    void uninstall_handler(const uint8_t irq) {
        irq_routines[irq] = nullptr;
    }
}

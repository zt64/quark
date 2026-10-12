#include <cstdint>

/**
 * IO APIC driver
 */
namespace apic::io {
    constexpr uint8_t IOAPIC_ID = 0x00;
    constexpr uint8_t IOAPIC_VERSION = 0x01;
    constexpr uint8_t IOAPIC_ARB_ID = 0x02;
    constexpr uint8_t IOAPIC_REDIR_TABLE_BASE = 0x3;

    static uint32_t read(void* io_apic_base, const uint32_t reg) {
        const auto io_apic_reg = static_cast<volatile uint32_t*>(io_apic_base);
        io_apic_reg[0] = reg;
        return io_apic_reg[4];
    }

    static void write(void* io_apic_base, const uint32_t reg, const uint32_t value) {
        const auto io_apic_reg = static_cast<volatile uint32_t*>(io_apic_base);
        io_apic_reg[0] = reg;
        io_apic_reg[4] = value;
    }
}

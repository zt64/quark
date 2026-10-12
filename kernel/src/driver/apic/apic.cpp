#include <arch/msr.hpp>
#include <driver/pic.hpp>
#include <driver/apic/apic.hpp>
#include <kernel/log.hpp>
#include <kernel/system.hpp>
#include <memory/paging.hpp>

/**
 * APIC driver
 */
namespace apic {
    constexpr uint8_t SPURIOUS_IVR = 0xF0;
    constexpr uint8_t TIMER_LVT = 0x320;
    constexpr uint8_t LVT_PERF = 0x340;
    constexpr uint16_t LVT_LINT0 = 0x350;
    constexpr uint16_t LVT_LINT1 = 0x360;
    constexpr uint16_t LVT_ERR = 0x370;
    constexpr uint16_t TMR_INIT_COUNT = 0x380;
    constexpr uint16_t TMR_CUR_COUNT = 0x390;
    constexpr uint8_t LOCAL_APIC_ID = 0x20;

    constexpr uint8_t IA32_APIC_BASE_MSR = 0x1B;
    constexpr uint8_t IA32_APIC_BASE_MSR_ENABLE = 0x800;

    static volatile uint32_t* lapic_regs = nullptr;

    static void set_apic_base(const uintptr_t apic_base) {
        uint32_t edx = 0;
        const uint32_t eax = (apic_base & 0XFFFFF0000) | IA32_APIC_BASE_MSR_ENABLE;

#ifdef __PHYSICAL_MEMORY_EXTENSION__
        edx = (apic_base >> 32) & 0x0f;
#endif

        arch::cpu_set_msr(IA32_APIC_BASE_MSR, eax, edx);
    }

    static uintptr_t get_apic_base() {
        uint32_t eax, edx;

        arch::cpu_get_msr(IA32_APIC_BASE_MSR, &eax, &edx);

#ifdef __PHYSICAL_MEMORY_EXTENSION__
        return (eax & 0xfffff000) | ((edx & 0x0f) << 32);
#else
        return (eax & 0xfffff000);
#endif
    }

    static uint32_t read_reg(const uint32_t reg) {
        return lapic_regs[reg / 4];
    }

    static void write_reg(const uint32_t reg, const uint32_t value) {
        lapic_regs[reg / 4] = value;
    }

    static void enable() {
        pic::disable();

        const uintptr_t phys_base = get_apic_base();
        set_apic_base(phys_base);

        lapic_regs = reinterpret_cast<volatile uint32_t*>(phys_base + paging::g_hhdm_offset);

        write_reg(SPURIOUS_IVR, 0x100 | 0xFF); // Enable APIC
    }

    void init() {
        if (!arch::cpu_has_apic()) {
            logger.warn("APIC not supported on this CPU, falling back to PIC");
            return;
        }

        enable();
    }
}

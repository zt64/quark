#pragma once

#include <cstdint>

namespace arch {
    constexpr uint32_t IA32_FS_BASE = 0xC0000100;

    struct cpuid_result {
        uint32_t eax;
        uint32_t ebx;
        uint32_t ecx;
        uint32_t edx;
    };

    cpuid_result cpuid(
        uint32_t leaf,
        uint32_t subleaf = 0
    );

    /**
     * Check if the CPU supports Model-Specific Registers (MSRs).
     * @return true if MSRs are supported, false otherwise.
     */
    bool cpu_has_msr();
    bool cpu_has_apic();
    void cpu_get_msr(uint32_t msr, uint32_t* lo, uint32_t* hi);
    void cpu_set_msr(uint32_t msr, uint32_t lo, uint32_t hi);
}

#pragma once
#include <cstdint>

namespace arch {
    void cpuid(
        uint32_t leaf,
        uint32_t subleaf = 0,
        uint32_t* a = nullptr,
        uint32_t* b = nullptr,
        uint32_t* c = nullptr,
        uint32_t* d = nullptr
    );
    bool cpu_has_msr();
    void cpu_get_msr(uint32_t msr, uint32_t* lo, uint32_t* hi);
    void cpu_set_msr(uint32_t msr, uint32_t lo, uint32_t hi);
}

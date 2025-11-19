#include "arch/msr.hpp"
#include "arch/cpuid.hpp"

namespace arch {
    void cpuid(
        uint32_t leaf,
        uint32_t subleaf,
        uint32_t* a,
        uint32_t* b,
        uint32_t* c,
        uint32_t* d
    ) {
        uint32_t eax, ebx, ecx, edx;
        asm volatile("cpuid"
            : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            : "a"(leaf), "c"(subleaf));
        if (a) *a = eax;
        if (b) *b = ebx;
        if (c) *c = ecx;
        if (d) *d = edx;
    }

    bool cpu_has_msr() {
        static uint32_t a, d; // eax, edx
        cpuid(1, 0, &a, &d);
        return d & CPUID_FEAT_EDX_MSR;
    }

    void cpu_get_msr(uint32_t msr, uint32_t* lo, uint32_t* hi) {
        uint32_t lo_val, hi_val;
        asm volatile("rdmsr" : "=a"(lo_val), "=d"(hi_val) : "c"(msr));
        if (lo) *lo = lo_val;
        if (hi) *hi = hi_val;
    }

    void cpu_set_msr(uint32_t msr, uint32_t lo, uint32_t hi) {
        asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
    }
}

#include "arch/msr.hpp"
#include "arch/cpuid.hpp"

namespace arch {
    cpuid_result cpuid(
        uint32_t leaf,
        uint32_t subleaf
    ) {
        cpuid_result r;
        asm volatile("cpuid"
            : "=a"(r.eax), "=b"(r.ebx), "=c"(r.ecx), "=d"(r.edx)
            : "a"(leaf), "c"(subleaf));

        return r;
    }

    bool cpu_has_msr() {
        const cpuid_result r = cpuid(1, 0);

        return r.edx & CPUID_FEAT_EDX_MSR;
    }

    bool cpu_has_apic() {
        const cpuid_result r = cpuid(1, 0);

        return r.edx & CPUID_FEAT_EDX_APIC;
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

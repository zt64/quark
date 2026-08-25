#pragma once

#include <cstdint>

constexpr uint32_t PIT_FREQUENCY = 1193182;

namespace timer {
    inline uint64_t timer_ticks = 0;

    void init(uint32_t frequency);

    void set_pit(uint32_t frequency);

    void wait(uint32_t ticks);

    void wait_ms(uint32_t ms);

    inline uint64_t get_ticks() { return timer_ticks; }

    inline void wait_for_tick(uint64_t previous) {
        // Wait for an interrupt-driven tick, but bound the number of HLT
        // iterations to avoid deadlocking if interrupts are not delivered
        // (e.g. due to PIC/APIC misconfiguration). If the bound is reached
        // the function returns and the caller can decide how to proceed.
        constexpr uint32_t max_hlts = 200000;
        uint32_t count = 0;
        while (static_cast<uint64_t>(timer_ticks) == previous) {
            asm volatile("hlt");
            if (++count >= max_hlts) {
                // fallback: small busy loop to yield a bit and return
                for (volatile uint32_t i = 0; i < 10000; ++i) asm volatile("pause");
                return;
            }
        }
    }
}

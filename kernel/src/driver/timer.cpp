#include "driver/timer.hpp"

#include <kernel/platform.hpp>

#include "driver/graphics/fb.hpp"
#include "driver/irq.hpp"
#include "driver/pic.hpp"
#include "driver/snd/sound.hpp"
#include "kernel/process/scheduler.hpp"
#include "kernel/system.hpp"

namespace timer {
    constexpr uint8_t PIT_CHANNEL_0 = 0x40;
    constexpr uint8_t PIT_CHANNEL_1 = 0x41;
    constexpr uint8_t PIT_CMD = 0x43;

    constexpr uint8_t TIMER_IRQ = 0;

    volatile uint64_t timer_base_ns = 0;
    volatile uint64_t timer_base_tsc = 0;
    uint64_t tsc_hz = 0;

    static uint64_t rdtsc() {
        uint32_t low, high;
        asm volatile("rdtsc" : "=a"(low), "=d"(high));
        return (static_cast<uint64_t>(high) << 32) | low;
    }

    static void timer_handler(const regs* r) {
        timer_ticks++;
        scheduler::reschedule();
        snd::snd_tick();
        pic::send_eoi(TIMER_IRQ);
    }

    void init(const uint32_t frequency) {
        irq::install_handler(TIMER_IRQ, timer_handler);
        set_pit(frequency);

        // Measure TSC frequency
        // const uint64_t start_tsc = rdtsc();
        // sleep(100); // Sleep for 100 ticks (1 second if frequency is 100
        // const uint64_t end_tsc = rdtsc();
        // tsc_hz = end_tsc - start_tsc;
    }

    void set_pit(const uint32_t frequency) {
        // Set PIT to fire at frequency in hz. ex: 100 Hz
        // PIT frequency = 1193182 Hz
        // Divisor for 100Hz = 1193182 / 100 = 11931 (0x2E9B)
        const uint32_t divisor = PIT_FREQUENCY / frequency;

        outb(PIT_CMD, 0x36);
        outb(PIT_CHANNEL_0, divisor & 0xFF);
        outb(PIT_CHANNEL_0, (divisor >> 8) & 0xFF);
    }

    /**
     * Wait for some time
     * @param ticks
     */
    void sleep(const uint32_t ticks) {
        const uint32_t eticks = static_cast<uint32_t>(timer_ticks) + ticks;
        while (static_cast<int32_t>(eticks - static_cast<uint32_t>(timer_ticks)) > 0) {
            // halt until the next interrupt
            asm volatile("hlt");
        }
    }

    void sleep_ms(const uint32_t ms) {
        sleep(ms);
    }

    void sleep_micro(const uint32_t micro) {
        sleep_ms(micro / 1000);
    }

    uint64_t ns_since_boot() {
        const uint64_t current_tsc = rdtsc();

        // Read the volatile base variables atomically (or disable interrupts briefly)
        platform::disable_interrupts();
        const uint64_t base_ns = timer_base_ns;
        const uint64_t base_tsc = timer_base_tsc;
        platform::enable_interrupts();

        if (current_tsc < base_tsc) {
            return base_ns; // Handle edge cases or slight out-of-order execution
        }

        const uint64_t tsc_delta = current_tsc - base_tsc;

        // Convert TSC delta to nanoseconds: (delta * 1,000,000,000) / tsc_hz
        // Using a fixed-point multiplier prevents expensive 64-bit division here
        const uint64_t ns_delta = (tsc_delta * 1000000000) / tsc_hz;

        return base_ns + ns_delta;
    }
}

#include "driver/timer.hpp"
#include "driver/fb.hpp"
#include "driver/pic.hpp"
#include "driver/sound.hpp"
#include "kernel/scheduler.hpp"
#include "kernel/system.hpp"

constexpr uint8_t PIT_CHANNEL_0 = 0x40;
constexpr uint8_t PIT_CHANNEL_1 = 0x41;
constexpr uint8_t PIT_CMD = 0x43;

namespace timer {
    static void timer_handler(const regs* r) {
        timer_ticks++;

        pic::send_eoi(0);

        scheduler::reschedule();

        snd::snd_tick();
    }

    void init(const uint32_t frequency) {
        irq::install_handler(0, timer_handler);
        set_pit(frequency);
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
    void wait(const uint32_t ticks) {
        const uint32_t eticks = static_cast<uint32_t>(timer_ticks) + ticks;
        while (static_cast<int32_t>(eticks - static_cast<uint32_t>(timer_ticks)) > 0) {
            // halt until the next interrupt
            asm volatile("hlt");
        }
    }

    void wait_ms(const uint32_t ms) {
        wait(ms / 10);
    }
}

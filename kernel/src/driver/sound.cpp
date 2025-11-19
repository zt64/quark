#include "driver/sound.hpp"
#include "driver/timer.hpp"
#include "kernel/system.hpp"

static bool beep_active = false;
static uint32_t beep_end_tick = 0;

namespace snd {
    void snd_play(const uint32_t frequency) {
        const uint32_t div = PIT_FREQUENCY / frequency;

        outb(0x43, 0xb6);
        outb(0x42, static_cast<uint8_t>(div & 0xFF));
        outb(0x42, static_cast<uint8_t>(div >> 8));

        if (const uint8_t tmp = inb(0x61); (tmp & 3) != 3) {
            outb(0x61, tmp | 3);
        }
    }

    void snd_stop() {
        const uint8_t tmp = inb(0x61) & 0xFC;
        outb(0x61, tmp);
    }

    void beep() {
        if (!beep_active) {
            snd_play(300);
            beep_active = true;
            beep_end_tick = timer::timer_ticks + 10;
        }
    }

    void snd_tick() {
        if (beep_active && timer::timer_ticks >= beep_end_tick) {
            snd_stop();
            beep_active = false;
        }
    }
}

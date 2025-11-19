#pragma once

#include <cstdint>

namespace snd {
    void snd_play(uint32_t frequency);

    void snd_stop();

    void beep();

    void snd_tick();
}

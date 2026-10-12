#pragma once

#include <cstdint>

namespace snd {
    class SoundDriver {
    public:
        virtual ~SoundDriver() = default;
        virtual void reset() = 0;
    };

    void snd_play(uint32_t frequency);

    void snd_stop();

    void beep();

    void snd_tick();
}

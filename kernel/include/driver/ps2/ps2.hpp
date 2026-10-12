#pragma once

#include <cstdint>

namespace ps2 {
    constexpr uint8_t PS2_COMMAND_PORT = 0x64;
    constexpr uint8_t PS2_STATUS_PORT = 0x64;
    constexpr uint8_t PS2_DATA_PORT = 0x60;

    void init();
}

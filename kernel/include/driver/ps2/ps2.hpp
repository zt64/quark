#pragma once
#include <cstdint>

constexpr uint8_t PS2_DATA_PORT = 0x60;
constexpr uint8_t PS2_COMMAND_PORT = 0x64;
constexpr uint8_t PS2_STATUS_PORT = 0x64;

namespace ps2 {
    void init();
}

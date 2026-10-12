#include <cstdint>
#include <driver/ps2/mouse.hpp>
#include <driver/ps2/ps2.hpp>
#include <kernel/system.hpp>

namespace ps2::mouse {
    constexpr uint8_t SET_SCALING_1_1 = 0xE6;
    constexpr uint8_t SET_SCALING_2_1 = 0xE7;
    constexpr uint8_t SET_RESOLUTION = 0xE8;
    constexpr uint8_t STATUS_REQUEST = 0xE9;
    constexpr uint8_t SET_STREAM_MODE = 0xEA;
    constexpr uint8_t READ_DATA = 0xEB;
    constexpr uint8_t RESET_WRAP_MODE = 0xEC;
    constexpr uint8_t SET_WRAP_MODE = 0xEE;
    constexpr uint8_t SET_REMOTE_MODE = 0xF0;
    constexpr uint8_t GET_DEVICE_ID = 0xF2;
    constexpr uint8_t SET_SAMPLE_RATE = 0xF3;
    constexpr uint8_t ENABLE_DATA_REPORTING = 0xF4;
    constexpr uint8_t DISABLE_DATA_REPORTING = 0xF5;
    constexpr uint8_t SET_DEFAULTS = 0xF6;
    constexpr uint8_t RESEND = 0xFE;
    constexpr uint8_t RESET = 0xFF;

    void set_sample_rate(const uint8_t rate) {
        outb(PS2_COMMAND_PORT, SET_SAMPLE_RATE);
        outb(PS2_DATA_PORT, rate);
    }

    void enable() {
        outb(PS2_COMMAND_PORT, ENABLE_DATA_REPORTING);
    }

    void disable() {
        outb(PS2_COMMAND_PORT, DISABLE_DATA_REPORTING);
    }

    void init() {
        set_sample_rate(0);
        enable();
    }
}

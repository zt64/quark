#include <cstdint>
#include <driver/timer.hpp>
#include <kernel/log.hpp>
#include <kernel/system.hpp>

// Sound Blaster 16 driver implementation
namespace sb16 {
    constexpr uint16_t DSP_MIXER_PORT = 0x224;
    constexpr uint16_t DSP_RESET_PORT = 0x225;
    constexpr uint16_t DSP_READ_DATA_PORT = 0x22A;
    constexpr uint16_t DSP_WRITE_DATA_PORT = 0x22C;
    constexpr uint16_t DSP_READ_STATUS_PORT = 0x22E;
    constexpr uint16_t DSP_INTERRUPT_PORT = 0x22F; // version 4.0+ only

    constexpr uint8_t CMD_SET_TIME_CONSTANT = 0x40;
    constexpr uint8_t CMD_SET_SAMPLE_RATE = 0x41;
    constexpr uint8_t CMD_TURN_ON_SPEAKER = 0xD1;
    constexpr uint8_t CMD_TURN_OFF_SPEAKER = 0xD3;
    constexpr uint8_t CMD_STOP_PLAYBACK_8 = 0xD0;
    constexpr uint8_t CMD_RESUME_PLAYBACK_8 = 0xD4;
    constexpr uint8_t CMD_STOP_PLAYBACK_16 = 0xD5;
    constexpr uint8_t CMD_RESUME_PLAYBACK_16 = 0xD6;
    constexpr uint8_t CMD_GET_DSP_VERSION = 0xE1;

    constexpr uint8_t CMD_SET_MIXER_VOLUME = 0x22;
    constexpr uint8_t CMD_SET_IRQ = 0x80;

    constexpr uint16_t BASE_IO = 0x220; // TODO: Get from PCI

    constexpr uint8_t IRQ = 5; // TODO: Get from PCI

    static void reset_dsp() {
        outb(BASE_IO + DSP_RESET_PORT, 1);
        timer::sleep_micro(3);
        outb(BASE_IO + DSP_RESET_PORT, 0);

        if (inb(BASE_IO + DSP_READ_STATUS_PORT) != 0xAA) {
            panic("SB16: DSP reset failed");
        }
    }

    static void set_irq(const uint8_t irq) {
        outb(BASE_IO + DSP_MIXER_PORT, 0x80); // Select IRQ register
        outb(BASE_IO + DSP_MIXER_PORT + 1, irq); // Set IRQ to the specified value
    }

    void play() {
        reset_dsp();
    }

    void init() {
        outb(BASE_IO + DSP_WRITE_DATA_PORT, CMD_GET_DSP_VERSION);
        logger.debug("SB16: DSP version: %02X", inb(BASE_IO + DSP_READ_DATA_PORT));

        set_irq(IRQ);
    }
}

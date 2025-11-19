#include "driver/ps2/ps2.hpp"
#include "kernel/system.hpp"

namespace ps2 {
    static void wait_input_clear() {
        while (inb(PS2_STATUS_PORT) & 0x02);
    }

    static void wait_output_full() {
        while (!(inb(PS2_STATUS_PORT) & 0x01));
    }

    void init() {
        // Disable both ports
        wait_input_clear();
        outb(PS2_COMMAND_PORT, 0xAD);
        wait_input_clear();
        outb(PS2_COMMAND_PORT, 0xA7);

        // Flush output buffer
        while (inb(PS2_STATUS_PORT) & 1) inb(PS2_DATA_PORT);

        // Get config byte
        wait_input_clear();
        outb(PS2_COMMAND_PORT, 0x20);
        wait_output_full();
        uint8_t config = inb(PS2_DATA_PORT);

        // Modify config byte
        config |= 0x01; // Enable first port interrupt
        config &= ~0x10; // Enable first port clock
        config &= ~0x40; // Disable translation
        config &= ~0x88; // Clear bits 3 and 7
        // Write config
        wait_input_clear();
        outb(PS2_COMMAND_PORT, 0x60);
        wait_input_clear();
        outb(PS2_DATA_PORT, config);

        // Controller self-test
        wait_input_clear();
        outb(PS2_COMMAND_PORT, 0xAA);
        wait_output_full();
        if (const uint8_t res = inb(PS2_DATA_PORT); res != 0x55) {
            panic("PS/2 Controller test fail! 0x%x", res);
        }

        // Enable port
        wait_input_clear();
        outb(PS2_COMMAND_PORT, 0xAE);

        // Reset keyboard
        // wait_input_clear();
        // outb(PS2_DATA_PORT, 0xFF);
        // wait_output_full();
        // inb(PS2_DATA_PORT);
        // wait_output_full();
        // inb(PS2_DATA_PORT);

        // Enable scanning
        wait_input_clear();
        outb(PS2_DATA_PORT, 0xF4);
        wait_output_full();
        inb(PS2_DATA_PORT);
    }
}

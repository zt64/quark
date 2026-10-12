#include <driver/ps2/keyboard.hpp>
#include <driver/ps2/mouse.hpp>
#include <driver/ps2/ps2.hpp>
#include <kernel/log.hpp>
#include <kernel/system.hpp>

namespace ps2 {
    constexpr uint8_t CMD_DISABLE_FIRST_PORT = 0xAD;
    constexpr uint8_t CMD_DISABLE_SECOND_PORT = 0xA7;
    constexpr uint8_t CMD_ENABLE_FIRST_PORT = 0xAE;
    constexpr uint8_t CMD_ENABLE_SECOND_PORT = 0xA8;
    constexpr uint8_t CMD_PS2_SECOND_PORT_WRITE = 0xD4;
    constexpr uint8_t CMD_READ_CONFIG_BYTE = 0x20;
    constexpr uint8_t CMD_WRITE_BYTE = 0x60;
    constexpr uint8_t CMD_TEST_CONTROLLER = 0xAA;
    constexpr uint8_t CMD_TEST_FIRST_PORT = 0xAB;
    constexpr uint8_t CMD_TEST_SECOND_PORT = 0xA9;
    constexpr uint8_t CMD_RESET_PORT = 0xFF;
    constexpr uint8_t CMD_IDENTIFY = 0xF2;

    constexpr uint8_t PS2_TEST_CONTROLLER_PASS = 0x55;
    constexpr uint8_t PS2_TEST_PORT_PASS = 0x00;
    constexpr uint8_t PS2_RESET_OK = 0xAA;

    constexpr uint8_t ACK = 0xFA;

    constexpr uint8_t ENABLE_SCANNING = 0xF4;
    constexpr uint8_t DISABLE_SCANNING = 0xF5;

    namespace {
        union ConfigByte {
            uint8_t raw;

            struct {
                uint8_t port1_irq_enabled : 1; // bit 0
                uint8_t port2_irq_enabled : 1; // bit 1
                uint8_t system_flag : 1; // bit 2
                uint8_t zero0 : 1; // bit 3, should be 0
                uint8_t port1_clock_disable : 1; // bit 4
                uint8_t port2_clock_disable : 1; // bit 5
                uint8_t translation_enable : 1; // bit 6
                uint8_t zero1 : 1; // bit 7, should be 0
            } bits;
        };
    }

    static bool wait_input_clear(const uint32_t iterations = 100000) {
        for (uint32_t i = 0; i < iterations; ++i) {
            if (!(inb(PS2_STATUS_PORT) & 0x02)) return true;
        }
        return false;
    }

    static bool wait_output_full(const uint32_t iterations = 100000) {
        for (uint32_t i = 0; i < iterations; ++i) {
            if (inb(PS2_STATUS_PORT) & 0x01) return true;
        }
        return false;
    }

    static void flush_output_buffer() {
        while (inb(PS2_STATUS_PORT) & 0x01) {
            inb(PS2_DATA_PORT);
        }
    }

    static bool write_byte(const uint8_t port, const uint8_t data) {
        if (!wait_input_clear()) {
            logger.warn("PS/2 Controller: timed out waiting for input buffer to clear");
            return false;
        }
        outb(port, data);
        return true;
    }

    static void write_cmd(const uint8_t command) {
        write_byte(PS2_COMMAND_PORT, command);
    }

    static uint8_t send_cmd(const uint8_t command) {
        write_cmd(command);
        wait_output_full();
        return inb(PS2_DATA_PORT);
    }

    static void read_config_byte(ConfigByte* config) {
        write_cmd(CMD_READ_CONFIG_BYTE);
        wait_output_full();
        config->raw = inb(PS2_DATA_PORT);
    }

    static void write_config_byte(const ConfigByte config) {
        write_cmd(CMD_WRITE_BYTE);
        write_byte(PS2_DATA_PORT, config.raw);
    }

    static uint8_t send_device(const uint8_t port, const uint8_t data) {
        if (port == 2) {
            write_cmd(CMD_PS2_SECOND_PORT_WRITE);
        }
        write_byte(PS2_DATA_PORT, data);
        wait_output_full();
        return inb(PS2_DATA_PORT); // should be ACK (0xFA)
    }

    static bool reset_device(const uint8_t port) {
        if (send_device(port, CMD_RESET_PORT) != ACK) {
            return false;
        }

        wait_output_full();
        if (inb(PS2_DATA_PORT) != PS2_RESET_OK) {
            return false;
        }

        if (port == 2) {
            wait_output_full();
            inb(PS2_DATA_PORT); // discard mouse device ID byte
        }

        return true;
    }

    struct DeviceId {
        uint8_t count; // 0, 1, or 2 — how many bytes were returned
        uint8_t bytes[2]; // ID byte(s), only `count` of these are valid
    };

    static bool identify_device(const uint8_t port, DeviceId* out) {
        if (send_device(port, DISABLE_SCANNING) != ACK) return false;
        if (send_device(port, CMD_IDENTIFY) != ACK) return false;

        out->count = 0;
        for (int i = 0; i < 2 && wait_output_full(); ++i) {
            out->bytes[out->count++] = inb(PS2_DATA_PORT);
        }
        return true;
    }

    void init() {
        // TODO: Check if PS/2 controller is present using ACPI tables, if not, skip initialization

        // Disable both ports
        write_cmd(CMD_DISABLE_FIRST_PORT);
        write_cmd(CMD_DISABLE_SECOND_PORT);

        // Flush output buffer
        flush_output_buffer();

        // Get config byte
        ConfigByte config;
        read_config_byte(&config);

        // Modify config byte
        config.bits.port1_irq_enabled = 1; // Enable first port interrupt
        config.bits.port1_clock_disable = 0; // Enable first port clock
        config.bits.translation_enable = 0; // Disable translation
        config.bits.zero0 = 0; // Clear bit 3
        config.bits.zero1 = 0; // Clear bit 7
        // Write config
        write_config_byte(config);

        // Controller self-test
        if (const uint8_t res = send_cmd(CMD_TEST_CONTROLLER); res != PS2_TEST_CONTROLLER_PASS) {
            panic("PS/2 Controller self-test failed! 0x%x", res);
        }

        // Enable port
        write_cmd(CMD_ENABLE_SECOND_PORT);

        read_config_byte(&config);

        bool dual_channel = false;

        if (config.bits.port2_clock_disable) {
            logger.warn("PS/2 Controller: Second port not available");
        } else {
            write_cmd(CMD_DISABLE_SECOND_PORT);
            config.bits.port2_irq_enabled = 0; // Disable second port interrupt
            config.bits.port2_clock_disable = 0; // Enable second port clock
            write_config_byte(config);
            dual_channel = true;
        }

        bool first_port_ok = true;
        if (const uint8_t res = send_cmd(CMD_TEST_FIRST_PORT); res != PS2_TEST_PORT_PASS) {
            logger.warn("PS/2 Controller: First port test fail! 0x%x", res);
            first_port_ok = false;
        }

        bool second_port_ok = false;
        if (dual_channel) {
            if (const uint8_t res = send_cmd(CMD_TEST_SECOND_PORT); res == PS2_TEST_PORT_PASS) {
                second_port_ok = true;
            } else {
                logger.warn("PS/2 Controller: Second port test fail! 0x%x", res);
            }
        }

        if (!first_port_ok && !second_port_ok) {
            // Continue booting without PS/2 support
            logger.warn("PS/2 Controller: no working ports");
            return;
        }

        if (first_port_ok) {
            write_cmd(CMD_ENABLE_FIRST_PORT);
            config.bits.port1_irq_enabled = 1; // Enable first port interrupt
        }

        if (dual_channel && second_port_ok) {
            write_cmd(CMD_ENABLE_SECOND_PORT);
            config.bits.port2_irq_enabled = 1; // Enable second port interrupt
        }

        write_config_byte(config);

        // Reset devices
        if (first_port_ok) {
            if (!reset_device(1)) {
                panic("PS/2 Keyboard failed to reset");
            }
        }

        if (dual_channel && second_port_ok) {
            if (!reset_device(2)) {
                panic("PS/2 Mouse failed to reset");
            }
        }

        DeviceId id;
        if (!identify_device(1, &id)) {
            panic("PS/2 Keyboard failed to identify");
        }

        if (id.count == 2) {
            logger.info("PS/2 Keyboard detected with ID: 0x%02X 0x%02X", id.bytes[0], id.bytes[1]);
        } else if (id.count == 1) {
            logger.info("PS/2 Keyboard detected with ID: 0x%02X", id.bytes[0]);
        } else {
            logger.info("PS/2 Keyboard detected (no ID, legacy AT keyboard)");
        }

        send_device(1, ENABLE_SCANNING);

        keyboard::init();
        // mouse::init();
    }
}

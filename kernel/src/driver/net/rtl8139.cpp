#include <cstdint>
#include <cstddef>
#include <kernel/system.hpp>

// RTL8139 Driver
// Reference: https://wiki.osdev.org/RTL8139
// Datasheet:
namespace rtl8139 {
    struct rtl8139_regs {
    };

    constexpr uint8_t ETH_FRAME_SIZE = 1518;

    constexpr uint8_t CONFIG_1 = 0x52;

    constexpr uint8_t MAC0 = 0x00;
    constexpr uint8_t MAR0 = 0x08;
    constexpr uint8_t RX_BUF = 0x30;
    constexpr uint8_t RX_EARLY_CNT = 0x34;
    constexpr uint8_t RX_EARLY_STATUS = 0x36;
    constexpr uint8_t CMD = 0x37;
    constexpr uint8_t CAPR = 0x38;
    constexpr uint8_t CBR = 0x3A; // Current Buffer Address
    constexpr uint8_t IMR = 0x3C; // Interrupt Mask Register
    constexpr uint8_t ISR = 0x3E; // Interrupt Status Register
    constexpr uint8_t TCR = 0x40; // Transmit Configuration Register
    constexpr uint8_t RCR = 0x44; // Receive Configuration Register
    constexpr uint8_t TIMER = 0x48; // Timer Register
    constexpr uint8_t CFG_9346 = 0x50; // Configuration Register
    constexpr uint8_t CFG_0 = 0x51; // Configuration Register
    constexpr uint8_t CFG_1 = 0x52; // Configuration Register

    struct header {
        uint8_t rok : 1;
        uint8_t align_err : 1;
        uint8_t crc_err : 1;
        uint8_t long_pkt : 1;
        uint8_t runt_pkt : 1;
        uint8_t is_err : 1;
        uint8_t reserved : 6;
        uint8_t bar : 1;
        uint8_t pam : 1;
        uint8_t mar : 1;
    };

    void init() {
        constexpr uint8_t IO_ADDR = 0x00; // todo: add io addr
        outb(CONFIG_1, 0x00);

        outl(IO_ADDR + RX_BUF, 0x00000000); // set receive buffer start address
        outb(IO_ADDR + CMD, 0x0C); // enable receiver and transmitter
    }

    void rtl8139_handler(uint8_t isr, uint64_t error, uint64_t irq) {
    }

    void receive_packet() {
    }

    void send_packet(const uint8_t* data, size_t length) {
    }
}

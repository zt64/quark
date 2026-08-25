#include "driver/serial.hpp"
#include "kernel/system.hpp"

namespace serial {
    static bool s_available = false;

    constexpr uint16_t PORT = 0x3F8;

    constexpr uint8_t IER_DISABLE_ALL = 0x00;
    constexpr uint8_t LCR_DLAB = 0x80;
    constexpr uint8_t LCR_8N1 = 0x03;
    constexpr uint8_t FCR_ENABLE_CLEAR_14 = 0xC7;
    constexpr uint8_t MCR_DTR_RTS_OUT2 = 0x0B;
    constexpr uint8_t MCR_LOOPBACK = 0x1E;
    constexpr uint8_t MCR_NORMAL_OPERATION = 0x0F;
    constexpr uint8_t UART_SELF_TEST_BYTE = 0xAE;

    constexpr uint8_t LSR_DATA_READY = 0x01;
    constexpr uint8_t LSR_TRANSMITTER_EMPTY = 0x20;

    int init() {
        outb(PORT + 1, IER_DISABLE_ALL); // Disable all interrupts
        outb(PORT + 3, LCR_DLAB); // Enable DLAB (set baud rate divisor)
        outb(PORT + 0, LCR_8N1); // Set divisor to 3 (lo byte) 38400 baud
        outb(PORT + 1, 0x00); //                  (hi byte)
        outb(PORT + 3, LCR_8N1); // 8 bits, no parity, one stop bit
        outb(PORT + 2, FCR_ENABLE_CLEAR_14); // Enable FIFO, clear them, with 14-byte threshold
        outb(PORT + 4, MCR_DTR_RTS_OUT2); // IRQs enabled, RTS/DSR set
        outb(PORT + 4, MCR_LOOPBACK); // Set in loopback mode, test the serial chip
        outb(PORT + 0, UART_SELF_TEST_BYTE); // Test serial chip (send byte 0xAE and check if serial returns same byte)
        // Check if serial is faulty (i.e: not same byte as sent)
        if (inb(PORT + 0) != UART_SELF_TEST_BYTE) {
            s_available = false;
        }

        // If serial is not faulty set it in normal operation mode
        outb(PORT + 4, MCR_NORMAL_OPERATION);

        // Mark initialized (we'll attempt to write but with a bounded wait)
        s_available = true;
        return 0;
    }

    bool received() {
        return inb(PORT + 5) & LSR_DATA_READY;
    }

    char read() {
        while (received() == 0) {
        }

        return static_cast<char>(inb(PORT));
    }

    bool transmitted() {
        return inb(PORT + 5) & LSR_TRANSMITTER_EMPTY;
    }

    void putchar(const char c) {
        if (!s_available) return;
        int timeout = 10000;
        while (timeout-- > 0 && !transmitted()) {
            asm volatile ("pause");
        }
        outb(PORT, c);
    }

    void print(const char* str) {
        if (!str) return;
        while (*str) {
            putchar(*str++);
        }
    }

    bool available() {
        return s_available;
    }
}

extern "C" void serial_putchar(char c) {
    serial::putchar(c);
}

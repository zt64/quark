#include "driver/cmos.hpp"

#include "driver/timer.hpp"
#include "kernel/system.hpp"

constexpr uint8_t NMI_disable_bit = 0;

constexpr uint8_t CMOS_PORT = 0x70;
constexpr uint8_t CMOS_RTC_SECONDS = 0x00;
constexpr uint8_t CMOS_RTC_STATUS_A = 0x0A;

uint32_t get_time() {
    while (true) {
        outb(CMOS_PORT, (NMI_disable_bit << 7) | CMOS_RTC_STATUS_A);
        const uint8_t status_a = inb(0x71);
        if ((status_a & 0x80) == 0) break; // UIP clear -> safe to read
    }

    outb(CMOS_PORT, (NMI_disable_bit << 7) | CMOS_RTC_SECONDS);
    return inb(0x71);
}

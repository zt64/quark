#include <driver/rtc.hpp>
#include <kernel/system.hpp>

namespace rtc {
    constexpr uint8_t CMOD_RTC_SECONDS = 0x00;
    constexpr uint8_t CMOD_RTC_MINUTES = 0x02;
    constexpr uint8_t CMOD_RTC_HOURS = 0x04;
    constexpr uint8_t CMOD_RTC_DAYS = 0x07;
    constexpr uint8_t CMOD_RTC_MONTHS = 0x08;
    constexpr uint8_t CMOD_RTC_YEARS = 0x09;
    constexpr uint8_t STATUS_REG_A = 0x0A;
    constexpr uint8_t STATUS_REG_B = 0x0B;

    constexpr uint8_t NMI_disable_bit = 0;

    constexpr uint8_t CMOS_PORT = 0x70;
    constexpr uint8_t CMOS_DATA_PORT = 0x71;

    uint32_t get_time() {
        while (true) {
            outb(CMOS_PORT, (NMI_disable_bit << 7) | STATUS_REG_A);
            const uint8_t status_a = inb(CMOS_DATA_PORT);
            if ((status_a & 0x80) == 0) break; // UIP clear -> safe to read
        }

        outb(CMOS_PORT, (NMI_disable_bit << 7) | CMOD_RTC_SECONDS);
        return inb(CMOS_DATA_PORT);
    }
}

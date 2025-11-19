#include <driver/rtc.hpp>

namespace rtc {
    constexpr uint8_t CMOD_RTC_SECONDS = 0x00;
    constexpr uint8_t CMOD_RTC_MINUTES = 0x02;
    constexpr uint8_t CMOD_RTC_HOURS = 0x04;
    constexpr uint8_t CMOD_RTC_DAYS = 0x07;
    constexpr uint8_t CMOD_RTC_MONTHS = 0x08;
    constexpr uint8_t CMOD_RTC_YEARS = 0x09;
    constexpr uint8_t STATUS_REG_A = 0x0A;
    constexpr uint8_t STATUS_REG_B = 0x0B;
}

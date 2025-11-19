#pragma once

#include <cstdint>

// Realtime Clock
namespace rtc {
    uint64_t get_second();
    uint64_t get_minute();
    uint64_t get_hour();
    uint64_t get_weekday();
    uint64_t get_day();
    uint64_t get_month();
    uint64_t get_year();
}
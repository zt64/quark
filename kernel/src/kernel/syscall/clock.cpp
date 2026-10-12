#include "kernel/syscall/internal.hpp"

#include <cstddef>
#include <cstdint>
#include <driver/timer.hpp>
#include <kernel/log.hpp>
#include <kernel/syscall/syscall.hpp>

typedef int64_t time_t;

constexpr uint8_t CLOCK_REALTIME = 0;
constexpr uint8_t CLOCK_MONOTONIC = 1;
constexpr uint8_t CLOCK_PROCESS_CPUTIME_ID = 2;
constexpr uint8_t CLOCK_THREAD_CPUTIME_ID = 3;
constexpr uint8_t CLOCK_MONOTONIC_RAW = 4;
constexpr uint8_t CLOCK_REALTIME_COARSE = 5;
constexpr uint8_t CLOCK_MONOTONIC_COARSE = 6;
constexpr uint8_t CLOCK_BOOTTIME = 7;
constexpr uint8_t CLOCK_REALTIME_ALARM = 8;
constexpr uint8_t CLOCK_BOOTTIME_ALARM = 9;
constexpr uint8_t CLOCK_TAI = 11;

static size_t sys_clock_get(const int clock, time_t* secs, long* nanos) {
    if (!secs || !nanos) {
        logger.error("clock_get: secs or nanos is null");
        return -1;
    }

    switch (clock) {
        case CLOCK_REALTIME:
            // timer::get_time(secs, nanos);
            *secs = 1234;
            break;
        case CLOCK_MONOTONIC: {
            const auto ns = timer::ns_since_boot();
            *secs = ns / 1'000'000'000;
            *nanos = ns % 1'000'000'000;
            break;
        }
        default:
            logger.error("clock_get: invalid clock %d", clock);
            return -1;
    }

    return 1;
}

SYSCALL_HANDLER(clock_get)

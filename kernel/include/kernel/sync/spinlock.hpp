#pragma once

#include <cstdint>
#include <kernel/platform.hpp>

namespace kernel {
    struct spinlock {
        uint32_t locked = 0;

        void lock() {
            platform::disable_interrupts();
            while (__atomic_test_and_set(&locked, __ATOMIC_ACQUIRE)) {
                asm volatile("pause");
            }
            platform::enable_interrupts();
        }

        void unlock() { __atomic_clear(&locked, __ATOMIC_RELEASE); }
        bool try_lock() { return !__atomic_test_and_set(&locked, __ATOMIC_ACQUIRE); }
    };
}

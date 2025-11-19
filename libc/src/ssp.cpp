#include <cstdint>
#include "kernel/system.hpp"

#if UINT32_MAX == UINTPTR_MAX
#define STACK_CHK_GUARD 0xe2dee396
#else
#define STACK_CHK_GUARD 0x595e9fbd94fda766
#endif

extern "C" {
    uintptr_t __stack_chk_guard = STACK_CHK_GUARD; // NOLINT(*-reserved-identifier)

    [[noreturn]] void __stack_chk_fail() { // NOLINT(*-reserved-identifier)
        panic("Stack smashing detected!\n");
    }

    [[noreturn]] void __stack_chk_fail_local() { // NOLINT(*-reserved-identifier)
        __stack_chk_fail();
    }
}


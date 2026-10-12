#pragma once

#include <cstdint>

namespace gdt {
    enum selector : uint16_t {
        NULL_SEG = 0x00,
        KERNEL_CS = 0x08,
        KERNEL_DS = 0x10,
        USER_CS = 0x1B, // (3 << 3) | RPL 3
        USER_DS = 0x23, // (4 << 3) | RPL 3
    };

    void init();
    void install_tss(uintptr_t base, uint32_t limit);
}

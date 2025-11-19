#pragma once

namespace gdt {
    void init();
    void install_tss(uintptr_t base, uint32_t limit);
}
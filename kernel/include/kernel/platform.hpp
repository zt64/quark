#pragma once

namespace platform {
    // Enables the FPU/SSE (CR0/CR4 bits). Must run before any floating-point
    // code executes, since Limine does not guarantee SSE is enabled on entry.
    void init_fpu();

    void init();
    void enable_interrupts();
}

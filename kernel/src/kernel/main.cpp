#include "driver/init.hpp"
#include "kernel/boot.hpp"
#include "kernel/log.hpp"
#include "kernel/memory_init.hpp"
#include "kernel/platform.hpp"
#include "kernel/stdio.hpp"
#include "kernel/userspace.hpp"

extern "C" [[noreturn]] void kmain() {
    platform::init_fpu();
    boot::init_early();
    logger.info("Initializing kernel...");
    platform::init();
    memory::init();
    drivers::init_early();
    init_stdio();
    platform::enable_interrupts();
    drivers::init_late();
    userspace::launch_init();
}

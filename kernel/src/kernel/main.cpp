#include <boot/limine/limine_requests.hpp>
#include <driver/graphics/fb.hpp>

#include "driver/init.hpp"
#include "kernel/boot.hpp"
#include "kernel/log.hpp"
#include "kernel/memory_init.hpp"
#include "kernel/platform.hpp"
#include "kernel/stdio.hpp"
#include "kernel/process/userspace.hpp"

extern "C" [[noreturn]] void kmain() {
    platform::init_fpu();
    boot::init_early();
    logger.info("Initializing kernel...");
    platform::init();
    logger.debug("Kernel initialized");
    memory::init();

    logger.debug("Memory initialized");
    const limine_framebuffer_response* framebuffer_response =
    limine_requests::framebuffer_request.response;

    if (!framebuffer_response || framebuffer_response->framebuffer_count == 0) {
        panic("No framebuffer provided");
    }

    fb::init(framebuffer_response->framebuffers[0]);
    logger.debug("Framebuffer initialized");

    drivers::init_early();
    init_stdio();
    platform::enable_interrupts();
    drivers::init_late();
    userspace::launch_init();
}

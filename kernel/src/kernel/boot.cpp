#include "kernel/boot.hpp"

#include "boot/limine/limine_requests.hpp"
#include "driver/fb.hpp"
#include "driver/serial.hpp"
#include "kernel/cmdline.hpp"
#include "kernel/log.hpp"
#include "kernel/system.hpp"

namespace boot {
    void init_early() {
        serial::init();

        if (!LIMINE_BASE_REVISION_SUPPORTED(limine_requests::limine_base_revision)) {
            panic("Unsupported Limine base revision");
        }

        const limine_framebuffer_response* framebuffer_response =
            limine_requests::framebuffer_request.response;

        if (!framebuffer_response || framebuffer_response->framebuffer_count == 0) {
            panic("No framebuffer provided");
        }

        screen::fb_init(framebuffer_response->framebuffers[0]);
        logger.debug("Framebuffer initialized");

        const limine_executable_cmdline_response* cmdline_response =
            limine_requests::executable_cmdline_request.response;
        if (cmdline_response) {
            parse_cmdline(cmdline_response->cmdline);
        }
    }
}
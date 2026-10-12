#include "kernel/boot.hpp"
#include <kernel/cmdline.hpp>
#include "boot/limine/limine_requests.hpp"
#include "driver/serial.hpp"
#include "driver/graphics/fb.hpp"
#include "kernel/system.hpp"

namespace boot {
    void init_early() {
        serial::init();

        if (!LIMINE_BASE_REVISION_SUPPORTED(limine_requests::limine_base_revision)) {
            panic("Unsupported Limine base revision");
        }

        const limine_executable_cmdline_response* cmdline_response = limine_requests::executable_cmdline_request.response;
        if (cmdline_response) {
            parse_cmdline(cmdline_response->cmdline);
        }
    }
}

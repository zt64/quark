#pragma once

#include "boot/limine/limine.h"

namespace limine_requests {
    extern volatile uint64_t limine_base_revision[];
    extern volatile limine_framebuffer_request framebuffer_request;
    extern volatile limine_executable_cmdline_request executable_cmdline_request;
    extern volatile limine_executable_address_request executable_addr_request;
    extern volatile limine_memmap_request memmap_request;
    extern volatile limine_hhdm_request hhdm_request;
    extern volatile limine_rsdp_request rsdp_request;
    extern volatile limine_smbios_request smbios_request;
    extern volatile limine_module_request module_request;
}
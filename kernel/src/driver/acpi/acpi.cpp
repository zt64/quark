#include "driver/acpi/acpi.hpp"
#include <uacpi/acpi.h>
#include <uacpi/uacpi.h>
#include "kernel/log.hpp"
#include "lib/string.hpp"

namespace acpi {
    int init() {
        const uacpi_status ret = uacpi_setup_early_table_access(tables, sizeof(tables));

        if (uacpi_unlikely_error(ret)) {
            logger.error("uacpi_setup_early_table_access error: %s", uacpi_status_to_string(ret));
            return -1;
        }

        return 0;
    }

    uacpi_table* find_table(const char* signature) {
        for (size_t i = 0; i < sizeof(tables) / sizeof(tables[0]); ++i) {
            if (tables[i].ptr == nullptr) {
                continue;
            }

            if (strncmp(signature, tables[i].hdr->signature, 4) == 0) {
                return &tables[i];
            }
        }

        return nullptr;
    }
}

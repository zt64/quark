#include <uacpi/uacpi.h>

#include "driver/acpi.hpp"
#include "kernel/log.hpp"

namespace acpi {
    int init() {
        const uacpi_status ret = uacpi_setup_early_table_access(tables, sizeof(tables));

        if (uacpi_unlikely_error(ret)) {
            logger.error("uacpi_setup_early_table_access error: %s", uacpi_status_to_string(ret));
            return -1;
        }

        return 0;
    }
}

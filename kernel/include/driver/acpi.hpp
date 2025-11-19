#pragma once

#include <cstddef>

#include <uacpi/tables.h>

namespace acpi {
    static uacpi_table tables[256] __attribute__((aligned(16)));

    int init();
}

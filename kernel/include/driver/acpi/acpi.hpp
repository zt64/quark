#pragma once

#include <uacpi/event.h>
#include <uacpi/tables.h>

namespace acpi {
    static uacpi_table tables[256] __attribute__((aligned(16)));

    struct event {
        volatile uint32_t counter;

        void reset() {
            counter = 0;
        }
    };

    int init();

    uacpi_table* find_table(const char* signature);

    void signal_event(const event& e);
}

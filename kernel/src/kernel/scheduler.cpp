#include "kernel/scheduler.hpp"

#include <kernel/log.hpp>
#include <kernel/process.hpp>
#include <kernel/tss.hpp>

namespace scheduler {
    void reschedule() {
        if (current == nullptr) return;

        task* next = current->next;

        if (next == nullptr || next == current) {
            return;
        }

        switch_to_task(next);
    }
}

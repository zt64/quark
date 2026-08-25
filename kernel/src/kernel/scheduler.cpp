#include "kernel/scheduler.hpp"
#include <kernel/process.hpp>

namespace scheduler {
    void reschedule() {
        if (current == nullptr) return;

        task* next = current->next;

        if (next == nullptr || next == current) {
            return;
        }

        switch_to_task(next);
    }

    void block_current(const void* channel) {
        current->state = SLEEPING;
        current->wait_channel = channel;

        task* next = current->next;
        while (next->state != RUNNING) {
            if (next == current) {
                // TODO: Implement proper task switching
                asm volatile("sti");
                asm volatile("hlt");
                asm volatile("cli");
                return;
            }
            next = next->next;
        }

        switch_to_task(next);
        // Resumes right here once this task is chosen again by the scheduler.
    }

    void unblock(const void* channel) {
        task* t = current;
        do {
            if (t->state == SLEEPING && t->wait_channel == channel) {
                t->state = RUNNING;
            }
            t = t->next;
        } while (t != current);
    }
}

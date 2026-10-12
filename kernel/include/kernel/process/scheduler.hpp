#pragma once

#include <kernel/process/process.hpp>

namespace scheduler {
    // TODO: Should be adjustable from a config
    constexpr uint32_t PID_MAX = 4096;

    /**
     * Registers a process with the scheduler, allowing it to be scheduled for execution.
     * @param p
     */
    void register_process(process_t* p);

    void schedule_task(task* t);
    void unschedule_task(const task* t);
    void reschedule();
    void block_current(const void* channel);
    void unblock(const void* channel);

    /**
     * Terminate a process and remove it from the scheduler, freeing its resources.
     * @param p
     */
    void terminate_proc(const process_t* p);

    [[noreturn]] void exit_current();
}

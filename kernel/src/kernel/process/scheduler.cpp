#include "kernel/process/scheduler.hpp"
#include <kernel/log.hpp>
#include <kernel/platform.hpp>
#include <kernel/process/process.hpp>
#include <memory/paging.hpp>

namespace scheduler {
    static process_t* processes[PID_MAX] = {};

    void register_process(process_t* p) {
        if (p->pid >= PID_MAX) {
            logger.error("scheduler: pid=%d exceeds PID_MAX", p->pid);
            return;
        }
        processes[p->pid] = p;
    }

    // Simple fixed-size circular buffer run queue. Holds only tasks that
    // are currently RUNNING (i.e. runnable) -- blocked/sleeping tasks are
    // removed from here entirely, not skipped-over via a state check.
    constexpr uint32_t RUN_QUEUE_CAP = 256;
    static task* run_queue[RUN_QUEUE_CAP] = {};
    static uint32_t rq_head = 0; // next to dequeue
    static uint32_t rq_tail = 0; // next free slot to enqueue into
    static uint32_t rq_count = 0;

    static void enqueue(task* t) {
        if (rq_count >= RUN_QUEUE_CAP) {
            logger.error("scheduler: run queue full, dropping tid=%d", t->tid);
            return;
        }
        run_queue[rq_tail] = t;
        rq_tail = (rq_tail + 1) % RUN_QUEUE_CAP;
        rq_count++;
    }

    static task* dequeue() {
        if (rq_count == 0) return nullptr;
        task* t = run_queue[rq_head];
        rq_head = (rq_head + 1) % RUN_QUEUE_CAP;
        rq_count--;
        return t;
    }

    void schedule_task(task* t) {
        t->state = RUNNING;
        enqueue(t);
    }

    void unschedule_task(const task* t) {
        for (uint32_t i = 0, idx = rq_head; i < rq_count; i++, idx = (idx + 1) % RUN_QUEUE_CAP) {
            if (run_queue[idx] == t) {
                for (uint32_t j = i; j < rq_count - 1; j++) {
                    const uint32_t a = (rq_head + j) % RUN_QUEUE_CAP;
                    const uint32_t b = (rq_head + j + 1) % RUN_QUEUE_CAP;
                    run_queue[a] = run_queue[b];
                }
                rq_count--;
                rq_tail = (rq_head + rq_count) % RUN_QUEUE_CAP;
                return;
            }
        }
    }

    void reschedule() {
        if (current == nullptr) return;
        if (rq_count == 0) return; // nothing else runnable, keep running current

        // Round-robin: put current back at the end of the queue (if it's
        // still runnable), then switch to whatever's at the front.
        if (current->state == RUNNING) {
            enqueue(current);
        }

        task* next = dequeue();
        if (next == nullptr || next == current) {
            return;
        }

        process::switch_to_task(next);
    }

    void block_current(const void* channel) {
        current->state = SLEEPING;
        current->wait_channel = channel;
        unschedule_task(current); // remove from run queue

        task* next = dequeue();
        if (next == nullptr) {
            // Nothing else runnable
            current->state = RUNNING;
            platform::enable_interrupts();
            asm volatile("hlt");
            platform::disable_interrupts();
            return;
        }

        process::switch_to_task(next);
    }

    void unblock(const void* channel) {
        // Walk every known process's task(s) looking for one sleeping on
        // this channel. For now, with one task per process, main_task
        // covers it; extend this if/when multi-threaded processes exist.
        for (const auto* p : processes) {
            if (p == nullptr || p->main_task == nullptr) continue;
            task* t = p->main_task;
            if (t->state == SLEEPING && t->wait_channel == channel) {
                t->state = RUNNING;
                enqueue(t);
            }
        }
    }

    void terminate_proc(const process_t* p) {
        logger.debug("Terminating process: pid=%d cr3=0x%lx", p->pid, p->cr3);

        if (p->main_task != nullptr) {
            unschedule_task(p->main_task);
        }

        unblock(reinterpret_cast<const void*>(static_cast<uintptr_t>(p->pid)));

        // paging::free_address_space(p->cr3);
        // TODO: free address space, kernel/user stacks, etc. -- deliberately
        // not doing that here yet, same reasoning as before: don't free
        // resources out from under code that might still be running on
        // them.
    }

    [[noreturn]] void exit_current() {
        task* next = dequeue();
        if (next == nullptr) {
            for (;;) {
                platform::enable_interrupts();
                asm volatile("hlt");
                platform::disable_interrupts();
            }
        }

        process::switch_to_task(next);
        __builtin_unreachable();
    }
}

#pragma once

#include <cstdint>
#include <kernel/sync/spinlock.hpp>

struct semaphore {
    kernel::spinlock guard;
    int32_t count;

    explicit semaphore(const int32_t initial) : count(initial) {
    }

    void acquire(); // guard.lock(); if count<=0 block_current(this); else count--; guard.unlock();
    void release(); // guard.lock(); count++; guard.unlock(); scheduler::unblock(this);
};

#pragma once
#include <kernel/process/scheduler.hpp>
#include <kernel/sync/spinlock.hpp>

struct mutex {
    kernel::spinlock lock;
    bool locked = false;

    void acquire() {
        lock.lock();
        while (locked) {
            lock.unlock();
            scheduler::block_current(this);
            lock.lock();
        }
        locked = true;
        lock.unlock();
    }

    void release() {
        lock.lock();
        locked = false;
        lock.unlock();

        scheduler::unblock(this);
    }
};

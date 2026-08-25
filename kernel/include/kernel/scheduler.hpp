#pragma once

namespace scheduler {
    enum BlockReason {
        STDIN
    };

    void reschedule();
    void block_current(const void* channel);
    void unblock(const void* channel);
}
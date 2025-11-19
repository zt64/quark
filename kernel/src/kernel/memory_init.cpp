#include "kernel/memory_init.hpp"
#include "kernel/log.hpp"
#include "memory/paging.hpp"
#include "memory/pmm.hpp"

namespace memory {
    void init() {
        paging::init();
        logger.info("Paging initialized");

        mem::init_pmm();
        logger.info("Physical memory manager initialized");
    }
}

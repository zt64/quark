#pragma once

#include <cstddef>

namespace mem {
    // Initialize the physical memory manager
    void init_pmm();

    // Allocate a single 4KB physical page (returns physical address)
    // The page is automatically zeroed
    void* allocate_physical_page();

    // Allocate multiple contiguous physical pages
    // Returns nullptr if not enough contiguous memory available
    void* allocate_physical_pages(size_t count);

    // Free a single physical page
    void free_physical_page(void* phys_addr);

    // Free multiple contiguous physical pages
    void free_physical_pages(void* phys_addr, size_t count);

    // Get memory statistics
    size_t get_free_pages();
    size_t get_used_pages();
    size_t get_total_pages();

    size_t get_free_memory();  // In bytes
    size_t get_used_memory();  // In bytes
    size_t get_total_memory(); // In bytes
}

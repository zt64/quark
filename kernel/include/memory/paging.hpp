#pragma once

#include <cstdint>

namespace paging {
    constexpr uint64_t PAGE_SIZE = 4096;
    constexpr uint64_t PAGE_PRESENT = 1ULL << 0;
    constexpr uint64_t PAGE_WRITABLE = 1ULL << 1;
    constexpr uint64_t PAGE_USER = 1ULL << 2;
    constexpr uint64_t PAGE_EXEC = 1ULL << 3;

    // Metadata supplied by Limine for the currently active address space.
    extern uint64_t g_kernel_phys_base;
    extern uint64_t g_kernel_virt_base;
    /**
     * Kernel size in bytes
     */
    extern uint64_t g_kernel_size;
    extern uint64_t g_hhdm_offset;
    extern uint64_t g_cr3_value;

    // Adopt Limine's active page tables. This does not modify CR3 or any mapping.
    void init();

    void switch_cr3(uint64_t cr3);

    /**
     * Create a new address space by allocating a new PML4 table.
     * @return the cr3 of the new address space, to be used with switch_cr3
     */
    uint64_t new_address_space();

    bool is_initialized();

    bool translate(uint64_t cr3, uintptr_t virt, uintptr_t& phys);
    // Walk the active four-level page tables without changing them.
    // Returns false when virt is not mapped.
    bool translate(uintptr_t virt, uintptr_t& phys);

    /**
     * Map a page into the provided cr3
     * @param cr3 The root pml4 table
     * @param virt
     * @param phys
     * @param flags
     * @return
     */
    bool map_page(uintptr_t cr3, uintptr_t virt, uintptr_t phys, uint64_t flags);

    // Map one currently unmapped 4 KiB page in the active address space.
    // The caller supplies page_writable and/or page_user as needed.
    bool map_page(uintptr_t virt, uintptr_t phys, uint64_t flags);

    bool unmap_page(uintptr_t cr3, uintptr_t virt);
    bool unmap_page(uintptr_t virt);
}

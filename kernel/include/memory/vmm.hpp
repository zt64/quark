#pragma once

#include <cstddef>
#include <cstdint>

constexpr uint8_t VM_FLAG_NONE = 0;
constexpr uint8_t VM_FLAG_WRITE = (1 << 0);
constexpr uint8_t VM_FLAG_EXEC = (1 << 1);
constexpr uint8_t VM_FLAG_USER = (1 << 2);
constexpr uint8_t VM_FLAG_MMIO = (1 << 3);

namespace vmm {
    /**
     * Find a free address in the provided address space (cr3) that can accommodate the requested length.
     * @param cr3 The CR3 value representing the address space to search for a free region.
     * @param length The length of the memory region to find.
     * @return The base address of the free region, or 0 if no suitable region is found.
     */
    uintptr_t find_free_addr(uint64_t cr3, size_t length);
    uintptr_t alloc(uint64_t cr3, size_t length, size_t flags);
    uintptr_t alloc(size_t length, size_t flags);

    /**
     * Track a memory region in the provided address space (cr3) with the specified base address, length, and flags.
     * This function is used to keep track of allocated memory regions for proper management and cleanup.
     * @param cr3 The CR3 value representing the address space to track the memory region in.
     * @param base The base address of the memory region to track.
     * @param length The length of the memory region to track.
     * @param flags The flags associated with the memory region to track.
     */
    void track(uint64_t cr3, uintptr_t base, size_t length, size_t flags);

    /**
     * Free a previously allocated memory region in the provided address space (cr3) at the specified pointer.
     * This function releases the memory region and updates the tracking information accordingly.
     * @param cr3 The CR3 value representing the address space to free the memory region from.
     * @param ptr The pointer to the memory region to free.
     */
    void vmm_free(uint64_t cr3, void* ptr);

    /**
     * Clone the address space from src_cr3 to dst_cr3. This function copies the memory mappings and contents from the source address space to the destination address space.
     * @param dst_cr3 The CR3 value representing the destination address space to clone to.
     * @param src_cr3 The CR3 value representing the source address space to clone from.
     */
    void clone_address_space(uint64_t dst_cr3, uint64_t src_cr3);
}

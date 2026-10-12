#pragma once

#include <cstddef>

extern "C" {
    /**
     * Allocate a block of kernel heap memory
     * @param size Number of bytes to allocate
     * @return Pointer to the allocated memory, or nullptr on failure
     */
    void* kalloc(size_t size);

    /**
     * Free a block of kernel heap memory previously returned by kalloc/krealloc
     * @param ptr Pointer to the memory block to free
     */
    void kfree(void* ptr);

    /**
     * Reallocate a block of kernel heap memory to a new size
     * @param ptr Pointer to the memory block to be reallocated
     * @param size The new size for the memory block
     * @return Pointer to the newly allocated memory block, or nullptr if the allocation fails or if size is zero
     */
    void* krealloc(void* ptr, size_t size);
}

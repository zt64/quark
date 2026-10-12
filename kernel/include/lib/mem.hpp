#pragma once

#include <cstddef>
#include <cstdint>

extern "C" {
    /**
     * Copy memory from source to destination
     * @param dst_ptr Destination pointer
     * @param src_ptr Source pointer
     * @param size Number of bytes to copy
     * @return Pointer to destination
     */
    void* memcpy(void* dst_ptr, const void* src_ptr, size_t size);

    /**
     * Set memory to a specific value
     * @param dst_ptr Destination pointer
     * @param val Value to set
     * @param count Number of bytes to set
     * @return Pointer to destination
     */
    void* memset(void* dst_ptr, uint8_t val, size_t count);

    /**
     * Move memory from source to destination
     * @param dst_ptr Destination pointer
     * @param src_ptr Source pointer
     * @param size Number of bytes to move
     * @return Pointer to destination
     */
    void* memmove(void* dst_ptr, const void* src_ptr, size_t size);

    /**
     * Compare two memory regions
     * @param a First memory region
     * @param b Second memory region
     * @param n Number of bytes to compare
     * @return 0 if equal, negative if a < b, positive if a > b
     */
    int32_t memcmp(const void* a, const void* b, size_t n);
}

#pragma once

#include <cstddef>
#include <cstdint>

extern "C" {
    void* memcpy(void* dst_ptr, const void* src_ptr, size_t size);
    void* memset(void* dst_ptr, uint8_t val, size_t count);
    void* memmove(void* dst_ptr, const void* src_ptr, size_t size);
    int32_t memcmp(const void* a, const void* b, size_t n);

    void* memcpy_fast(void* dst_ptr, const void* src_ptr, size_t n);
    void* memmove_fast(void* dst_ptr, const void* src_ptr, size_t n);
    void* memset_fast(void* dst_ptr, uint8_t val, size_t n);
}

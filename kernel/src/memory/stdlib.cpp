#include "lib/stdlib.hpp"

#include <cstdint>

constexpr size_t heap_max = 1024 * 1024 * 16; // 16 MB
alignas(16) static uint8_t heap_buf[heap_max];

static uint8_t* heap_end = heap_buf + heap_max;
static uint8_t* heap_ptr = heap_buf;

static size_t align_up(const size_t v, const size_t align) {
    return (v + (align - 1)) & ~(align - 1);
}

void* malloc(const size_t size) {
    if (!size) return nullptr;

    const size_t asize = align_up(size, 8);
    const uintptr_t aligned_ptr = align_up(reinterpret_cast<uintptr_t>(heap_ptr), 8);
    auto* p = reinterpret_cast<uint8_t*>(aligned_ptr);

    if (p + asize > heap_end) {
        return nullptr;
    }
    heap_ptr = p + asize;
    return p;
}

void free(void* block) {
    if (!block) return; // Already freed
    (void)block;
}

void* realloc(void* ptr, const size_t size) {
    // TODO: Implement memory reallocation
    (void)ptr;
    (void)size;
    return nullptr;
}
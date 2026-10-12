#include <cstddef>
#include "memory/heap.hpp"

void* operator new(const size_t size) {
    return kalloc(size);
}

void* operator new[](const size_t size) {
    return kalloc(size);
}

void operator delete(void* p) {
    kfree(p);
}

void operator delete[](void* p) {
    kfree(p);
}

void operator delete(void* p, const size_t size) {
    (void)size;
    kfree(p);
}

void operator delete[](void* p, const size_t size) {
    (void)size;
    kfree(p);
}

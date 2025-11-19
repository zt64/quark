#include <cstddef>
#include "stdlib.hpp"

void* operator new(const size_t size) {
    return malloc(size);
}

void* operator new[](const size_t size) {
    return malloc(size);
}

void operator delete(void* p) {
    free(p);
}

void operator delete[](void* p) {
    free(p);
}

void operator delete(void* p, const size_t size) {
    (void)size;
    free(p);
}

void operator delete[](void* p, const size_t size) {
    (void)size;
    free(p);
}

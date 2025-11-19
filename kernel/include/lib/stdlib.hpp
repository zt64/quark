#pragma once

#include <cstddef>

extern "C" {
    void* malloc(size_t size);
    void free(void* block);
    void* realloc(void* ptr, size_t size);
}

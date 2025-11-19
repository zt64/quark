#pragma once

#include <cstddef>
#include <stdint-gcc.h>

#define VM_FLAG_NONE 0
#define VM_FLAG_WRITE (1 << 0)
#define VM_FLAG_EXEC (1 << 1)
#define VM_FLAG_USER (1 << 2)
#define VM_FLAG_MMIO (1 << 3)

namespace vmm {
    uintptr_t find_free_addr(size_t length);
    uintptr_t vmm_alloc(uint64_t cr3, size_t length, size_t flags, const void* arg);
    uintptr_t vmm_alloc(size_t length, size_t flags, const void* arg);
    void vmm_free(void* ptr);
}

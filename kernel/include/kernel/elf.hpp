#pragma once

#include <cstdint>

namespace elf {
    struct segment {
        uintptr_t virt_base;
        uintptr_t phys_base;
        uint32_t page_count;
    };
    class ElfImage {
    public:
        void* entry_point;
        uintptr_t base_address;
        uint32_t image_size;
        segment segments[16];
        uint32_t segment_count;
    };

    ElfImage* load_file(uintptr_t cr3, uintptr_t addr);
    void unload_file(const ElfImage* image);
}

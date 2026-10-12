#pragma once

#include <cstdint>

namespace elf {
    /**
     * A segment in an elf image
     */
    struct segment {
        uintptr_t virt_base;
        uintptr_t phys_base;
        uint32_t page_count;
    };

    /**
     * An elf image
     */
    class ElfImage {
    public:
        void* entry_point;
        uintptr_t base_address;
        uint32_t image_size;
        segment segments[16];
        uint32_t segment_count;
        uintptr_t phdr_addr;
        uint16_t phentsize;
        uint16_t phnum;
    };

    /**
     * Load an elf file into the provided cr3 from the given address. The file is expected to be in memory at the given address.
     * @param cr3
     * @param addr
     * @return
     */
    ElfImage* load_file(uintptr_t cr3, uintptr_t addr);
    void unload_file(const ElfImage* image);
}

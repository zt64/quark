#include "memory/pmm.hpp"

#include <cstddef>

#include "lib/mem.hpp"
#include "boot/limine/limine_requests.hpp"
#include "kernel/log.hpp"
#include "kernel/system.hpp"
#include "memory/paging.hpp" // for g_kernel_phys_base / g_kernel_size / g_hhdm_offset

namespace mem {
    constexpr size_t PAGE_SIZE = 4096;

    // Bitmap-based physical memory manager
    static uint8_t* bitmap = nullptr;
    static size_t bitmap_size = 0;
    static size_t total_pages = 0;
    static size_t free_pages = 0;
    static size_t used_pages = 0;

    static uint64_t memory_top = 0;
    static uint64_t hhdm_offset = 0;

    // Helper: convert physical address to bitmap index
    static inline size_t phys_to_index(const uint64_t phys) {
        return phys / PAGE_SIZE;
    }

    // Helper: convert bitmap index to physical address
    static inline uint64_t index_to_phys(const size_t index) {
        return index * PAGE_SIZE;
    }

    // Helper: check if a bit is set in the bitmap
    static inline bool bitmap_test(const size_t index) {
        const size_t byte = index / 8;
        const size_t bit = index % 8;
        if (!bitmap) {
            logger.error("PMM: bitmap_test called but bitmap == nullptr (index=%zu)", index);
            return true; // treat as used to be safe
        }
        if (byte >= bitmap_size) {
            logger.error("PMM: bitmap_test OOB: index=%zu byte=%zu bitmap_size=%zu", index, byte, bitmap_size);
            return true; // treat as used
        }
        return (bitmap[byte] & (1 << bit)) != 0;
    }

    // Helper: set a bit in the bitmap (mark as used)
    static inline void bitmap_set(const size_t index) {
        const size_t byte = index / 8;
        const size_t bit = index % 8;
        if (!bitmap) {
            logger.error("PMM: bitmap_set called but bitmap == nullptr (index=%zu)", index);
            return;
        }
        if (byte >= bitmap_size) {
            logger.error("PMM: bitmap_set OOB: index=%zu byte=%zu bitmap_size=%zu", index, byte, bitmap_size);
            return;
        }
        bitmap[byte] |= (1 << bit);
    }

    // Helper: clear a bit in the bitmap (mark as free)
    static inline void bitmap_clear(const size_t index) {
        const size_t byte = index / 8;
        const size_t bit = index % 8;
        if (!bitmap) {
            logger.error("PMM: bitmap_clear called but bitmap == nullptr (index=%zu)", index);
            return;
        }
        if (byte >= bitmap_size) {
            logger.error("PMM: bitmap_clear OOB: index=%zu byte=%zu bitmap_size=%zu", index, byte, bitmap_size);
            return;
        }
        bitmap[byte] &= ~(1 << bit);
    }

    void init_pmm() {
        if (limine_requests::memmap_request.response == nullptr) {
            panic("Memory map request failed");
        }

        hhdm_offset = limine_requests::hhdm_request.response->offset;
        const auto* memmap = limine_requests::memmap_request.response;

        // Calculate total usable memory and highest USABLE address
        uint64_t total_usable_bytes = 0;
        uint64_t highest_usable_addr = 0;

        for (uint64_t i = 0; i < memmap->entry_count; i++) {
            const auto* entry = memmap->entries[i];

            if (entry->type == LIMINE_MEMMAP_USABLE) {
                total_usable_bytes += entry->length;
                const uint64_t top = entry->base + entry->length;
                if (top > highest_usable_addr) {
                    highest_usable_addr = top;
                }
            }
        }

        memory_top = highest_usable_addr;
        total_pages = (memory_top + PAGE_SIZE - 1) / PAGE_SIZE;
        bitmap_size = (total_pages + 7) / 8;

        logger.info(
            "PMM: Highest usable address = 0x%llx (%llu MB)",
            memory_top,
            memory_top / (1024 * 1024)
        );
        logger.info(
            "PMM: Total usable memory = %llu MB",
            total_usable_bytes / (1024 * 1024)
        );

        // Find location for bitmap
        for (uint64_t i = 0; i < memmap->entry_count; i++) {
            const auto* entry = memmap->entries[i];

            if (entry->type == LIMINE_MEMMAP_USABLE) {
                // Align candidate location to a page boundary
                const uint64_t region_start = (entry->base + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1); // align_up
                const uint64_t region_end = (entry->base + entry->length) & ~(PAGE_SIZE - 1); // align_down
                if (region_end > region_start && (region_end - region_start) >= bitmap_size) {
                    const uint64_t bitmap_phys = region_start; // place bitmap at start of usable region (page-aligned)
                    bitmap = reinterpret_cast<uint8_t*>(bitmap_phys + hhdm_offset);
                    logger.info(
                        "PMM: Bitmap at phys 0x%lx (virt 0x%lx), size %zu KB (from region start 0x%lx len=0x%lx)",
                        bitmap_phys,
                        reinterpret_cast<uint64_t>(bitmap),
                        bitmap_size / 1024,
                        entry->base,
                        entry->length
                    );
                    break;
                }
            }
        }

        if (bitmap == nullptr) {
            panic("Could not allocate bitmap");
        }

        // Mark everything as used
        memset(bitmap, 0xFF, bitmap_size);
        free_pages = 0;
        used_pages = total_pages;

        // Mark usable regions as free
        for (uint64_t i = 0; i < memmap->entry_count; i++) {
            const auto* entry = memmap->entries[i];

            if (entry->type == LIMINE_MEMMAP_USABLE) {
                // Only mark whole pages fully contained in the region as free
                const uint64_t start = (entry->base + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1); // align_up
                const uint64_t end = (entry->base + entry->length) & ~(PAGE_SIZE - 1); // align_down
                for (uint64_t addr = start; addr + PAGE_SIZE <= end; addr += PAGE_SIZE) {
                    const size_t index = phys_to_index(addr);
                    if (index < total_pages && bitmap_test(index)) {
                        bitmap_clear(index);
                        free_pages++;
                        used_pages--;
                    }
                }
            }
        }

        // Mark bitmap pages as used
        const size_t bitmap_phys = reinterpret_cast<uint64_t>(bitmap) - hhdm_offset;
        const size_t bitmap_pages = (bitmap_size + PAGE_SIZE - 1) / PAGE_SIZE;

        for (size_t i = 0; i < bitmap_pages; i++) {
            const size_t index = phys_to_index(bitmap_phys + i * PAGE_SIZE);
            if (index < total_pages && !bitmap_test(index)) {
                bitmap_set(index);
                free_pages--;
                used_pages++;
            }
        }

        // Reserve kernel pages (if paging provided kernel region info)
        if (paging::g_kernel_phys_base != 0 && paging::g_kernel_size != 0) {
            const uint64_t kstart = paging::g_kernel_phys_base;
            const uint64_t kend = kstart + paging::g_kernel_size;
            const uint64_t start_page = phys_to_index((kstart + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
            const uint64_t end_page = phys_to_index(kend & ~(PAGE_SIZE - 1));
            for (uint64_t p = start_page; p < end_page && p < total_pages; ++p) {
                if (!bitmap_test(p)) {
                    bitmap_set(p);
                    if (free_pages > 0) free_pages--;
                    used_pages++;
                }
            }
        }

        logger.info(
            "PMM: Free = %zu MB, Used = %zu MB, Total tracked = %zu MB",
            (free_pages * PAGE_SIZE) / (1024 * 1024),
            (used_pages * PAGE_SIZE) / (1024 * 1024),
            (total_pages * PAGE_SIZE) / (1024 * 1024)
        );
    }

    void* allocate_physical_page() {
        // Prefer allocating pages above 1MiB to avoid low-memory conflicts
        constexpr size_t skip_bytes = 0x100000; // 1 MiB
        size_t preferred_start = phys_to_index(skip_bytes);
        if (preferred_start >= total_pages) preferred_start = 0;

        // Find first free page in bitmap
        // First scan preferred high region [preferred_start..total_pages)
        for (size_t i = preferred_start; i < total_pages; i++) {
            if (!bitmap_test(i)) {
                bitmap_set(i);
                free_pages--;
                used_pages++;

                const uint64_t phys = index_to_phys(i);

                // Zero out the page
                const auto virt = reinterpret_cast<void*>(phys + hhdm_offset);
                memset(virt, 0, PAGE_SIZE);

                return reinterpret_cast<void*>(phys);
            }
        }

        // If not found, wrap and scan low region [0..preferred_start)
        if (preferred_start != 0) {
            for (size_t i = 0; i < preferred_start; ++i) {
                if (!bitmap_test(i)) {
                    bitmap_set(i);
                    free_pages--;
                    used_pages++;

                    const uint64_t phys = index_to_phys(i);
                    const auto virt = reinterpret_cast<void*>(phys + hhdm_offset);

                    memset(virt, 0, PAGE_SIZE);

                    return reinterpret_cast<void*>(phys);
                }
            }
        }

        logger.error("PMM: Out of physical memory! free=%zu used=%zu total=%zu", free_pages, used_pages, total_pages);

        return nullptr;
    }

    void* allocate_physical_pages(const size_t count) {
        if (count == 0) return nullptr;
        if (count == 1) return allocate_physical_page();

        // Find a contiguous block of free pages
        size_t found = 0;
        size_t start_index = 0;

        for (size_t i = 0; i < total_pages; i++) {
            if (!bitmap_test(i)) {
                if (found == 0) {
                    start_index = i;
                }
                found++;

                if (found == count) {
                    // Found enough contiguous pages
                    for (size_t j = 0; j < count; j++) {
                        bitmap_set(start_index + j);
                        free_pages--;
                        used_pages++;
                    }

                    const uint64_t phys = index_to_phys(start_index);

                    // Zero out all pages
                    const auto virt = reinterpret_cast<void*>(phys + hhdm_offset);
                    memset(virt, 0, PAGE_SIZE * count);

                    return reinterpret_cast<void*>(phys);
                }
            } else {
                found = 0;
            }
        }

        logger.error("PMM: Could not find %zu contiguous physical pages", count);
        return nullptr;
    }

    void free_physical_page(void* phys_addr) {
        const auto phys = reinterpret_cast<uint64_t>(phys_addr);
        const size_t index = phys_to_index(phys);

        if (index >= total_pages) {
            logger.warn("PMM: Attempted to free invalid physical address 0x%lx", phys);
            return;
        }

        if (!bitmap_test(index)) {
            logger.warn("PMM: Attempted to free already-free page at 0x%lx", phys);
            return;
        }

        bitmap_clear(index);
        free_pages++;
        used_pages--;
    }

    void free_physical_pages(void* phys_addr, const size_t count) {
        const auto phys = reinterpret_cast<uint64_t>(phys_addr);

        for (size_t i = 0; i < count; i++) {
            free_physical_page(reinterpret_cast<void*>(phys + i * PAGE_SIZE));
        }
    }

    // Get statistics
    size_t get_free_pages() {
        return free_pages;
    }

    size_t get_used_pages() {
        return used_pages;
    }

    size_t get_total_pages() {
        return total_pages;
    }

    size_t get_free_memory() {
        return free_pages * PAGE_SIZE;
    }

    size_t get_used_memory() {
        return used_pages * PAGE_SIZE;
    }

    size_t get_total_memory() {
        return total_pages * PAGE_SIZE;
    }
}

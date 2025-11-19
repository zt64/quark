#include "memory/paging.hpp"

#include "boot/limine/limine_requests.hpp"
#include "kernel/system.hpp"
#include "kernel/log.hpp"
#include "memory/pmm.hpp"

namespace paging {
    namespace {
        constexpr uint64_t page_size_bit = 1ULL << 7;
        // Mask to extract physical address
        constexpr uint64_t page_frame_mask = 0x000FFFFFFFFFF000ULL;
        constexpr uint64_t large_page_frame_mask = 0x000FFFFFFFE00000ULL;
        constexpr uint64_t huge_page_frame_mask = 0x000FFFFFC0000000ULL;
        constexpr uint64_t large_page_size = 2ULL * 1024 * 1024;
        constexpr uint64_t huge_page_size = 1024ULL * 1024 * 1024;

        uint64_t* pml4_table = nullptr;

        uint16_t pml4_index(const uint64_t address) {
            return (address >> 39) & 0x1FF;
        }

        uint16_t pdpt_index(const uint64_t address) {
            return (address >> 30) & 0x1FF;
        }

        uint16_t pd_index(const uint64_t address) {
            return (address >> 21) & 0x1FF;
        }

        uint16_t pt_index(const uint64_t address) {
            return (address >> 12) & 0x1FF;
        }

        uint64_t* physical_to_virtual(const uint64_t physical) {
            return reinterpret_cast<uint64_t*>(physical + g_hhdm_offset);
        }

        bool is_present(const uint64_t entry) {
            return (entry & PAGE_PRESENT) != 0;
        }

        /**
         * Get the address of the table, creating it if not already created
         * @param entry The entry to check or create
         * @param flags The flags to apply if creating a new table
         * @return The address of the next table or nullptr if not present
         */
        uint64_t* get_or_create_next_table(uint64_t& entry, const uint64_t flags) {
            if (!is_present(entry)) {
                // Create a new table
                const auto phys = reinterpret_cast<uintptr_t>(mem::allocate_physical_page());

                if (phys == 0) return nullptr;

                entry = phys | PAGE_PRESENT | PAGE_WRITABLE | (flags & PAGE_USER);
            }

            if ((entry & page_size_bit) != 0) return nullptr;

            if ((flags & PAGE_USER) != 0) {
                entry |= PAGE_USER;
            }

            return physical_to_virtual(entry & page_frame_mask);
        }
    }

    uint64_t g_kernel_phys_base = 0;
    uint64_t g_kernel_virt_base = 0;
    uint64_t g_kernel_size = 0;
    uint64_t g_hhdm_offset = 0;
    uint64_t g_cr3_value = 0;

    extern "C" uint64_t kernel_start[];
    extern "C" uint64_t kernel_end[];
    extern "C" uint64_t kernel_text_start[];
    extern "C" uint64_t kernel_text_end[];
    extern "C" uint64_t kernel_rodata_start[];
    extern "C" uint64_t kernel_rodata_end[];
    extern "C" uint64_t kernel_data_start[];
    extern "C" uint64_t kernel_data_end[];

    void init() {
        if (limine_requests::executable_addr_request.response == nullptr) {
            panic("Kernel address request not found");
        }

        if (limine_requests::hhdm_request.response == nullptr) {
            panic("HHDM request not found");
        }

        g_hhdm_offset = limine_requests::hhdm_request.response->offset;
        g_kernel_phys_base = limine_requests::executable_addr_request.response->physical_base;
        g_kernel_virt_base = limine_requests::executable_addr_request.response->virtual_base;
        g_kernel_size = (kernel_text_end - kernel_text_start) + (kernel_rodata_end - kernel_rodata_start) + (
            kernel_data_end - kernel_data_start);

        asm volatile("mov %%cr3, %0" : "=r"(g_cr3_value));
        g_cr3_value &= page_frame_mask;
        pml4_table = physical_to_virtual(g_cr3_value);

        logger.info(
            "Paging adopted: CR3 phys=0x%lx, HHDM=0x%lx, kernel=%lu KiB",
            g_cr3_value,
            g_hhdm_offset,
            g_kernel_size / 1024
        );
    }

    constexpr uint32_t address_spaces_max = 1024;
    uint64_t address_spaces[address_spaces_max];

    uint64_t new_address_space() {
        if (!is_initialized()) {
            logger.error("new_address_space: paging not initialized");
            return 0;
        }

        const auto phys = reinterpret_cast<uintptr_t>(mem::allocate_physical_page());
        const auto virt = physical_to_virtual(phys);

        // map kernel pml4 to higher half of task local pml4 256-512
        for (uint64_t i = 256; i < 512; i++) {
            virt[i] = pml4_table[i];
        }

        for (uint32_t i = 0; i < address_spaces_max; i++) {
            if (address_spaces[i] == 0) {
                address_spaces[i] = phys;
                break;
            }
        }

        return phys;
    }

    void switch_cr3(const uint64_t cr3) {
        asm volatile("mov %0, %%cr3" :: "r"(cr3));
    }

    bool is_initialized() {
        return pml4_table != nullptr;
    }

    bool translate(const uint64_t cr3, const uintptr_t virt, uintptr_t& phys) {
        if (cr3 == 0) return false;

        const uint64_t phys_base = cr3 & page_frame_mask;
        const uint64_t* root_table = physical_to_virtual(cr3 & page_frame_mask);

        const uint64_t pml4e = root_table[pml4_index(virt)];
        if (!is_present(pml4e)) return false;

        const uint64_t* pdpt = physical_to_virtual(pml4e & page_frame_mask);
        const uint64_t pdpte = pdpt[pdpt_index(virt)];
        if (!is_present(pdpte)) return false;

        if ((pdpte & page_size_bit) != 0) {
            phys = (pdpte & huge_page_frame_mask) + (virt & (huge_page_size - 1));
            return true;
        }

        const uint64_t* pd = physical_to_virtual(pdpte & page_frame_mask);
        const uint64_t pde = pd[pd_index(virt)];
        if (!is_present(pde)) return false;
        if ((pde & page_size_bit) != 0) {
            phys = (pde & large_page_frame_mask) + (virt & (large_page_size - 1));
            return true;
        }

        const uint64_t* pt = physical_to_virtual(pde & page_frame_mask);
        const uint64_t pte = pt[pt_index(virt)];
        if (!is_present(pte)) {
            return false;
        }

        phys = (pte & page_frame_mask) + (virt & (PAGE_SIZE - 1));
        return true;
    }

    /**
     * Translate a virtual address to a physical address
     * @param virt
     * @param phys
     * @return
     */
    bool translate(const uintptr_t virt, uintptr_t& phys) {
        if (!is_initialized()) return false;

        return translate(g_cr3_value, virt, phys);
    }

    bool map_page(const uintptr_t cr3, const uintptr_t virt, const uintptr_t phys, const uint64_t flags) {
        if (cr3 == 0) return false;

        const uint64_t phys_base = cr3 & page_frame_mask;
        uint64_t* root_table = physical_to_virtual(cr3 & page_frame_mask);

        auto* pdpt = get_or_create_next_table(root_table[pml4_index(virt)], flags);
        if (pdpt == nullptr) {
            logger.error("map_page: failed to get or create PDPT for virt=0x%lx", virt);
            return false;
        }

        auto* pd = get_or_create_next_table(pdpt[pdpt_index(virt)], flags);
        if (pd == nullptr) {
            logger.error("map_page: failed to get or create PD for virt=0x%lx", virt);
            return false;
        }

        auto* pt = get_or_create_next_table(pd[pd_index(virt)], flags);
        if (pt == nullptr) {
            logger.error("map_page: failed to get or create PT for virt=0x%lx", virt);
            return false;
        }

        uint64_t& pte = pt[pt_index(virt)];
        if (is_present(pte)) {
            return false;
        }

        pte = phys | PAGE_PRESENT | (flags & (PAGE_WRITABLE | PAGE_USER));
        asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
        return true;
    }

    bool map_page(const uintptr_t virt, const uintptr_t phys, const uint64_t flags) {
        if (!is_initialized()
            || (virt & (PAGE_SIZE - 1)) != 0
            || (phys & (PAGE_SIZE - 1)) != 0) {
            return false;
        }

        return map_page(g_cr3_value, virt, phys, flags);
    }

    bool unmap_page(const uint64_t cr3, const uintptr_t virt) {
        const uint64_t phys_base = cr3 & page_frame_mask;
        uint64_t* root_virt = physical_to_virtual(phys_base);
        uint64_t* root_table = physical_to_virtual(cr3 & page_frame_mask);

        auto* pdpt = physical_to_virtual(root_table[pml4_index(virt)] & page_frame_mask);
        if (pdpt == nullptr) {
            logger.error("unmap_page: failed to get PDPT for virt=0x%lx", virt);
            return false;
        }

        auto* pd = physical_to_virtual(pdpt[pdpt_index(virt)] & page_frame_mask);
        if (pd == nullptr) {
            logger.error("unmap_page: failed to get PD for virt=0x%lx", virt);
            return false;
        }

        auto* pt = physical_to_virtual(pd[pd_index(virt)] & page_frame_mask);
        if (pt == nullptr) {
            logger.error("unmap_page: failed to get PT for virt=0x%lx", virt);
            return false;
        }

        uint64_t& pte = pt[pt_index(virt)];
        if (!is_present(pte)) {
            return false;
        }

        pte = 0;
        asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
        return true;
    }

    bool unmap_page(const uintptr_t virt) {
        if (!is_initialized()) {
            return false;
        }

        return unmap_page(g_cr3_value, virt);
    }
}

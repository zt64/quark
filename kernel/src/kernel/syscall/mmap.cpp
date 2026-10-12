#include "kernel/syscall/internal.hpp"

#include <driver/graphics/fb.hpp>
#include <kernel/log.hpp>
#include <kernel/process/process.hpp>
#include <kernel/syscall/syscall.hpp>
#include <lib/mem.hpp>
#include <memory/paging.hpp>
#include <memory/pmm.hpp>
#include <memory/vmm.hpp>

#include "kernel/system.hpp"

static size_t align_up(const size_t v, const size_t align) {
    return (v + (align - 1)) & ~(align - 1);
}

constexpr int MMAP_FD_FRAMEBUFFER = 3;
constexpr uint8_t MAP_ANONYMOUS = 0x20;
constexpr uint8_t MAP_FIXED = 0x10;

static void* sys_mmap(
    const void* addr, const size_t length, const int prot, const int flags, const int fd
) {
    (void)prot;

    if (length == 0) {
        return nullptr;
    }

    const bool is_fixed = (flags & MAP_FIXED) != 0 && addr != nullptr;

    uintptr_t user_base;

    if (addr == nullptr) {
        user_base = vmm::find_free_addr(current->proc->cr3, length);
    } else {
        user_base = reinterpret_cast<uintptr_t>(addr);
    }

    if (fd == MMAP_FD_FRAMEBUFFER) {
        const auto fb_virt = reinterpret_cast<uintptr_t>(fb::framebuffer.addr);

        uintptr_t fb_phys;
        if (!paging::translate(fb_virt, fb_phys)) {
            return nullptr;
        }

        const size_t size = fb::framebuffer.pitch * fb::framebuffer.height;
        const uintptr_t phys_page = fb_phys & ~(paging::PAGE_SIZE - 1);
        const size_t page_offset = fb_phys & (paging::PAGE_SIZE - 1);
        const size_t map_size = align_up(size + page_offset, paging::PAGE_SIZE);

        for (size_t map_offset = 0; map_offset < map_size; map_offset += paging::PAGE_SIZE) {
            paging::map_page(
                current->proc->cr3,
                user_base + map_offset,
                phys_page + map_offset,
                paging::PAGE_USER | paging::PAGE_WRITABLE);
        }

        vmm::track(current->proc->cr3, user_base, map_size, VM_FLAG_WRITE | VM_FLAG_USER);

        return reinterpret_cast<void*>(user_base + page_offset);
    }

    const size_t map_size = align_up(length, paging::PAGE_SIZE);

    for (size_t map_offset = 0; map_offset < map_size; map_offset += paging::PAGE_SIZE) {
        const uintptr_t virt = user_base + map_offset;

        uintptr_t existing_phys;
        if (is_fixed && paging::translate(current->proc->cr3, virt, existing_phys)) {
            // Already backed by the earlier reservation -- nothing to do.
            continue;
        }

        void* phys = pmm::allocate_physical_page();
        if (!phys) {
            logger.error("sys_mmap: out of physical memory mapping 0x%lx", virt);

            for (size_t rollback = 0; rollback < map_offset; rollback += paging::PAGE_SIZE) {
                paging::unmap_page(current->proc->cr3, user_base + rollback);
            }
            return nullptr;
        }

        if (!paging::map_page(
            current->proc->cr3,
            virt,
            reinterpret_cast<uintptr_t>(phys),
            paging::PAGE_USER | paging::PAGE_WRITABLE)) {
            logger.error("sys_mmap: failed to map page at 0x%lx", virt);

            pmm::free_physical_pages(phys, 1);
            for (size_t rollback = 0; rollback < map_offset; rollback += paging::PAGE_SIZE) {
                paging::unmap_page(current->proc->cr3, user_base + rollback);
            }
            return nullptr;
        }
    }

    // Only the initial (hint-less) reservation needs bookkeeping -- a
    // MAP_FIXED "commit" call reuses that same tracked region, so tracking
    // it again would create bogus overlapping entries that corrupt future
    // vmm::find_free_addr() lookups.
    if (!is_fixed) {
        vmm::track(current->proc->cr3, user_base, map_size, VM_FLAG_WRITE | VM_FLAG_USER);
    }

    return reinterpret_cast<void*>(user_base);
}

static size_t sys_munmap(const void* addr, const size_t length, const int prot, const int flags) {
    if (addr == nullptr || length == 0) {
        return -1;
    }

    return 0;
}

static void copy_from_user(const uint64_t cr3, const void* src, void* dst, const size_t length) {
    uintptr_t phys;
    if (!paging::translate(cr3, reinterpret_cast<uintptr_t>(src), phys)) {
        panic("copy_from_user: failed to translate user pointer");
    }

    auto* user_ptr = reinterpret_cast<void*>(phys + paging::g_hhdm_offset);
    memcpy(dst, user_ptr, length);
}

SYSCALL_HANDLER(mmap);
SYSCALL_HANDLER(munmap);

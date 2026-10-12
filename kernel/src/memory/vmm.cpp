#include "memory/vmm.hpp"
#include <kernel/log.hpp>
#include "memory/heap.hpp"
#include "lib/mem.hpp"
#include "memory/paging.hpp"
#include "memory/pmm.hpp"

namespace vmm {
    namespace {
        typedef struct vm_object {
            uintptr_t base;
            size_t length;
            size_t flags;
            vm_object* next;
        } vm_object;

        constexpr size_t MAX_ADDRESS_SPACES = 64;
        struct address_space_entry {
            uint64_t cr3;
            vm_object* objs;
        };
        address_space_entry g_spaces[MAX_ADDRESS_SPACES] = {};

        vm_object** objs_for(const uint64_t cr3) {
            for (auto& [entry_cr3, objs] : g_spaces) {
                if (entry_cr3 == cr3) {
                    return &objs;
                }
            }
            for (auto& [entry_cr3, objs] : g_spaces) {
                if (entry_cr3 == 0) {
                    entry_cr3 = cr3;
                    objs = nullptr;
                    return &objs;
                }
            }
            logger.error("vmm: address space table exhausted");
            return nullptr;
        }
    }

    static uint64_t convert_x86_64_vm_flags(const size_t flags) {
        uint64_t value = 0;
        if (flags & VM_FLAG_WRITE) value |= paging::PAGE_WRITABLE;
        if (flags & VM_FLAG_USER) value |= paging::PAGE_USER;
        if ((flags & VM_FLAG_EXEC) == 0) value |= paging::PAGE_EXEC;
        return value;
    };

    uintptr_t find_free_addr(const uint64_t cr3, const size_t length) {
        constexpr uintptr_t min_base = 0x10000000;
        vm_object** list = objs_for(cr3);
        if (list == nullptr) return 0;

        const vm_object* current = *list;
        const vm_object* prev = nullptr;
        uintptr_t found = min_base;

        while (current != nullptr) {
            const uintptr_t base = (prev == nullptr ? min_base : prev->base + prev->length);
            if (base + length <= current->base) {
                found = base;
                break;
            }
            prev = current;
            current = current->next;
        }

        if (current == nullptr && prev != nullptr) {
            found = prev->base + prev->length;
        } else if (current == nullptr && prev == nullptr) {
            found = min_base;
        }

        return found;
    }

    uintptr_t alloc(const uint64_t cr3, size_t length, const size_t flags) {
        length = ((length + paging::PAGE_SIZE - 1) / paging::PAGE_SIZE) * paging::PAGE_SIZE;

        vm_object** list = objs_for(cr3);
        if (list == nullptr) {
            logger.error("vmm_alloc: failed to get vm_object list for cr3=0x%lx", cr3);
            return 0;
        }

        constexpr uintptr_t min_base = 0x10000000;
        vm_object* current = *list;
        vm_object* prev = nullptr;
        uintptr_t found = min_base;

        while (current != nullptr) {
            const uintptr_t base = (prev == nullptr ? min_base : prev->base + prev->length);
            if (base + length <= current->base) {
                found = base;
                break;
            }
            prev = current;
            current = current->next;
        }

        if (current == nullptr && prev != nullptr) {
            found = prev->base + prev->length;
        } else if (current == nullptr && prev == nullptr) {
            found = min_base;
        }

        auto* latest = static_cast<vm_object*>(kalloc(sizeof(vm_object)));
        if (latest == nullptr) {
            return 0;
        }

        latest->base = found;
        latest->length = length;
        latest->flags = flags;

        const auto phys_base = reinterpret_cast<uintptr_t>(pmm::allocate_physical_pages(length / paging::PAGE_SIZE));
        if (phys_base == 0) {
            kfree(latest);
            return 0;
        }

        const uint64_t page_flags = convert_x86_64_vm_flags(flags);
        for (size_t offset = 0; offset < length; offset += paging::PAGE_SIZE) {
            if (!paging::map_page(cr3, latest->base + offset, phys_base + offset, page_flags)) {
                for (size_t rollback = 0; rollback < offset; rollback += paging::PAGE_SIZE) {
                    paging::unmap_page(cr3, latest->base + rollback);
                }
                pmm::free_physical_pages(reinterpret_cast<void*>(phys_base), offset / paging::PAGE_SIZE);
                kfree(latest);
                return 0;
            }
        }

        if (prev == nullptr) {
            *list = latest;
        } else {
            prev->next = latest;
        }
        latest->next = current;

        return latest->base;
    }

    void track(const uint64_t cr3, const uintptr_t base, const size_t length, const size_t flags) {
        vm_object** list = objs_for(cr3);
        if (list == nullptr) return;

        vm_object* current_obj = *list;
        vm_object* prev = nullptr;
        while (current_obj != nullptr && current_obj->base < base) {
            prev = current_obj;
            current_obj = current_obj->next;
        }

        auto* latest = static_cast<vm_object*>(kalloc(sizeof(vm_object)));
        if (latest == nullptr) {
            logger.error("vmm::track: failed to allocate vm_object for 0x%lx", base);
            return;
        }
        latest->base = base;
        latest->length = length;
        latest->flags = flags;
        latest->next = current_obj;

        if (prev == nullptr) {
            *list = latest;
        } else {
            prev->next = latest;
        }
    }

    void vmm_free(const uint64_t cr3, void* ptr) {
        vm_object** list = objs_for(cr3);
        if (list == nullptr) return;

        vm_object* prev = nullptr;
        vm_object* vmob = *list;

        while (vmob != nullptr) {
            if (vmob->base == reinterpret_cast<uintptr_t>(ptr)) {
                break;
            }
            prev = vmob;
            vmob = vmob->next;
        }

        if (vmob == nullptr) {
            logger.error("vmm_free: vmob is nullptr");
            return;
        }

        const size_t length = vmob->length;
        const size_t page_count = length / paging::PAGE_SIZE;

        uintptr_t phys_base = 0;
        if (!paging::translate(cr3, vmob->base, phys_base)) {
            logger.error("vmm_free: failed to translate base address");
            return;
        }

        for (size_t offset = 0; offset < length; offset += paging::PAGE_SIZE) {
            if (!paging::unmap_page(cr3, vmob->base + offset)) {
                logger.error("vmm_free: failed to unmap 0x%lx", vmob->base + offset);
                return;
            }
        }

        pmm::free_physical_pages(reinterpret_cast<void*>(phys_base), page_count);

        if (prev == nullptr) {
            *list = vmob->next;
        } else {
            prev->next = vmob->next;
        }

        kfree(vmob);
    }

    void clone_address_space(const uint64_t dst_cr3, const uint64_t src_cr3) {
        vm_object** src_list = objs_for(src_cr3);
        if (src_list == nullptr) return;

        for (const vm_object* obj = *src_list; obj != nullptr; obj = obj->next) {
            const size_t page_count = obj->length / paging::PAGE_SIZE;
            const auto phys_base = reinterpret_cast<uintptr_t>(pmm::allocate_physical_pages(page_count));
            if (phys_base == 0) {
                logger.error("clone_address_space: failed to allocate pages for 0x%lx", obj->base);
                continue;
            }

            const uint64_t page_flags = convert_x86_64_vm_flags(obj->flags);
            for (size_t offset = 0; offset < obj->length; offset += paging::PAGE_SIZE) {
                uintptr_t src_phys;
                if (!paging::translate(src_cr3, obj->base + offset, src_phys)) {
                    logger.error("clone_address_space: failed to translate src 0x%lx", obj->base + offset);
                    continue;
                }
                memcpy(
                    reinterpret_cast<void*>(phys_base + offset + paging::g_hhdm_offset),
                    reinterpret_cast<const void*>(src_phys + paging::g_hhdm_offset),
                    paging::PAGE_SIZE
                );
                paging::map_page(dst_cr3, obj->base + offset, phys_base + offset, page_flags);
            }

            track(dst_cr3, obj->base, obj->length, obj->flags);
        }
    }
}
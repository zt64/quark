#include "memory/vmm.hpp"
#include <cstdint>
#include <kernel/log.hpp>
#include "lib/stdlib.hpp"
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
    }

    static vm_object* vm_objs = nullptr;

    static uint64_t convert_x86_64_vm_flags(const size_t flags) {
        uint64_t value = 0;
        if (flags & VM_FLAG_WRITE)
            value |= paging::PAGE_WRITABLE;
        if (flags & VM_FLAG_USER)
            value |= paging::PAGE_USER;
        if ((flags & VM_FLAG_EXEC) == 0)
            value |= paging::PAGE_EXEC;
        return value;
    };

    uintptr_t find_free_addr(const size_t length) {
        constexpr uintptr_t min_base = paging::PAGE_SIZE;
        const vm_object* current = vm_objs;
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

    uintptr_t vmm_alloc(const uint64_t cr3, size_t length, const size_t flags, const void* arg) {
        (void)arg;
        length = ((length + paging::PAGE_SIZE - 1) / paging::PAGE_SIZE) * paging::PAGE_SIZE;

        constexpr uintptr_t min_base = paging::PAGE_SIZE;
        vm_object* current = vm_objs;
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

        auto* latest = static_cast<vm_object*>(malloc(sizeof(vm_object)));
        if (latest == nullptr) {
            return 0;
        }

        latest->base = found;
        latest->length = length;
        latest->flags = flags;

        const auto phys_base = reinterpret_cast<uintptr_t>(mem::allocate_physical_pages(length / paging::PAGE_SIZE));
        if (phys_base == 0) {
            return 0;
        }

        const uint64_t page_flags = convert_x86_64_vm_flags(flags);
        for (size_t offset = 0; offset < length; offset += paging::PAGE_SIZE) {
            if (!paging::map_page(cr3, latest->base + offset, phys_base + offset, page_flags)) {
                for (size_t rollback = 0; rollback < offset; rollback += paging::PAGE_SIZE) {
                    paging::unmap_page(cr3, latest->base + rollback);
                }
                mem::free_physical_pages(reinterpret_cast<void*>(phys_base), offset / paging::PAGE_SIZE);
                free(latest);
                return 0;
            }
        }

        if (prev == nullptr) {
            vm_objs = latest;
        } else {
            prev->next = latest;
        }
        latest->next = current;

        return latest->base;

    }

    uintptr_t vmm_alloc(size_t length, const size_t flags, const void* arg) {
        (void)arg;
        length = ((length + paging::PAGE_SIZE - 1) / paging::PAGE_SIZE) * paging::PAGE_SIZE;

        constexpr uintptr_t min_base = paging::PAGE_SIZE;
        vm_object* current = vm_objs;
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

        auto* latest = static_cast<vm_object*>(malloc(sizeof(vm_object)));
        if (latest == nullptr) {
            return 0;
        }

        latest->base = found;
        latest->length = length;
        latest->flags = flags;

        const auto phys_base = reinterpret_cast<uintptr_t>(mem::allocate_physical_pages(length / paging::PAGE_SIZE));
        if (phys_base == 0) {
            return 0;
        }

        const uint64_t page_flags = convert_x86_64_vm_flags(flags);
        for (size_t offset = 0; offset < length; offset += paging::PAGE_SIZE) {
            if (!paging::map_page(latest->base + offset, phys_base + offset, page_flags)) {
                for (size_t rollback = 0; rollback < offset; rollback += paging::PAGE_SIZE) {
                    paging::unmap_page(latest->base + rollback);
                }
                mem::free_physical_pages(reinterpret_cast<void*>(phys_base), offset / paging::PAGE_SIZE);
                free(latest);
                return 0;
            }
        }

        if (prev == nullptr) {
            vm_objs = latest;
        } else {
            prev->next = latest;
        }
        latest->next = current;

        return latest->base;
    }

    void vmm_free(void* ptr) {
        vm_object* prev = nullptr;
        vm_object* vmob = vm_objs;

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
        if (!paging::translate(vmob->base, phys_base)) {
            logger.error("vmm_free: failed to translate base address");
            return;
        }

        for (size_t offset = 0; offset < length; offset += paging::PAGE_SIZE) {
            if (!paging::unmap_page(vmob->base + offset)) {
                logger.error("vmm_free: failed to unmap 0x%lx", vmob->base + offset);
                return;
            }
        }

        mem::free_physical_pages(reinterpret_cast<void*>(phys_base), page_count);

        if (prev == nullptr) {
            vm_objs = vmob->next;
        } else {
            prev->next = vmob->next;
        }

        free(vmob);
    }
}

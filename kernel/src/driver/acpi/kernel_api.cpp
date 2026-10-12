#include "uacpi/kernel_api.h"
#include <driver/timer.hpp>
#include <driver/acpi/acpi.hpp>
#include <driver/pci/pci.hpp>
#include <kernel/sync/mutex.hpp>
#include "boot/limine/limine_requests.hpp"
#include "kernel/log.hpp"
#include "lib/mem.hpp"
#include "lib/string.hpp"
#include "memory/heap.hpp"

uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr* out_rsdp_address) {
    if (!out_rsdp_address) return UACPI_STATUS_INVALID_ARGUMENT;
    if (!limine_requests::rsdp_request.response) return UACPI_STATUS_NOT_FOUND;

    const auto limine_addr = reinterpret_cast<uintptr_t>(limine_requests::rsdp_request.response->address);
    uint64_t phys = limine_addr;

    if (limine_requests::hhdm_request.response) {
        const uint64_t hhdm = limine_requests::hhdm_request.response->offset;
        if (limine_addr >= hhdm) {
            phys = limine_addr - hhdm;
        }
    }

    *out_rsdp_address = phys;
    return UACPI_STATUS_OK;
}

void* uacpi_kernel_map(const uacpi_phys_addr addr, const uacpi_size len) {
    if (!limine_requests::hhdm_request.response) return nullptr;
    const uint64_t hhdm = limine_requests::hhdm_request.response->offset;

    auto phys = static_cast<uint64_t>(addr);
    if (phys >= hhdm) phys = phys - hhdm;
    const uacpi_phys_addr phys_addr = phys;

    const uacpi_phys_addr page = phys_addr & ~static_cast<uacpi_phys_addr>(0xFFFULL);
    const size_t offset = phys_addr - page;

    const uintptr_t virt_page = hhdm + page;
    return reinterpret_cast<void*>(virt_page + offset);
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void uacpi_kernel_unmap(void* addr, const uacpi_size len) {
    (void)addr;
    (void)len;

    // TODO: Implement unmap if necessary. For now this is a no-op since we are using a direct mapping (HHDM).
}

void uacpi_kernel_log(const uacpi_log_level level, const uacpi_char* msg) {
    const auto uacpi_msg = reinterpret_cast<const char*>(msg ? msg : "");
    const auto m = static_cast<char*>(kalloc(strlen(uacpi_msg)));

    strcpy(m, uacpi_msg);
    m[strlen(uacpi_msg) - 1] = '\0';

    constexpr auto fmt = "uACPI: %s";

    switch (level) {
        case UACPI_LOG_DEBUG:
            logger.debug(fmt, m);
            break;
        case UACPI_LOG_TRACE:
            logger.debug(fmt, m);
            break;
        case UACPI_LOG_INFO:
            logger.info(fmt, m);
            break;
        case UACPI_LOG_WARN:
            logger.warn(fmt, m);
            break;
        case UACPI_LOG_ERROR:
            logger.error(fmt, m);
            break;
        default:
            logger.info(fmt, m);
    }
}

#ifndef UACPI_BAREBONES_MODE

uacpi_status uacpi_kernel_pci_device_open(const uacpi_pci_address address, uacpi_handle* out_handle) {
    memcpy(*out_handle, &address, sizeof(uacpi_pci_address));
    return UACPI_STATUS_OK;
}

void uacpi_kernel_pci_device_close(uacpi_handle) {
    // No-op
}

uacpi_status uacpi_kernel_pci_read8(
    const uacpi_handle device, const uacpi_size offset, uacpi_u8* value
) {
    const auto addr = static_cast<uacpi_pci_address*>(device);

    *value = pci::read_conf8(addr->bus, addr->device, addr->function, offset);
    return UACPI_STATUS_OK;
};

uacpi_status uacpi_kernel_pci_read16(
    const uacpi_handle device, const uacpi_size offset, uacpi_u16* value
) {
    const auto addr = static_cast<uacpi_pci_address*>(device);

    *value = pci::read_conf16(addr->bus, addr->device, addr->function, offset);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_read32(
    const uacpi_handle device, const uacpi_size offset, uacpi_u32* value
) {
    const auto addr = static_cast<uacpi_pci_address*>(device);

    *value = pci::read_conf32(addr->bus, addr->device, addr->function, offset);
    return UACPI_STATUS_OK;
};

template <typename T>
static uacpi_status pci_write(const uacpi_handle device, const uacpi_size offset, const T value) {
    const auto addr = static_cast<uacpi_pci_address*>(device);

    switch (sizeof(T)) {
        case 1:
            pci::write_conf8(addr->bus, addr->device, addr->function, offset, value);
            break;
        case 2:
            pci::write_conf16(addr->bus, addr->device, addr->function, offset, value);
            break;
        case 4:
            pci::write_conf32(addr->bus, addr->device, addr->function, offset, value);
            break;
        default:
            return UACPI_STATUS_INVALID_ARGUMENT;
    }

    return UACPI_STATUS_OK;
};

uacpi_status uacpi_kernel_pci_write8(const uacpi_handle device, const uacpi_size offset, const uacpi_u8 value) {
    return pci_write(device, offset, value);
};

uacpi_status uacpi_kernel_pci_write16(const uacpi_handle device, const uacpi_size offset, const uacpi_u16 value) {
    return pci_write(device, offset, value);
};

uacpi_status uacpi_kernel_pci_write32(const uacpi_handle device, const uacpi_size offset, const uacpi_u32 value) {
    return pci_write(device, offset, value);
};

uacpi_status uacpi_kernel_io_map(const uacpi_io_addr base, const uacpi_size size, uacpi_handle* out_handle) {
    *out_handle = reinterpret_cast<uacpi_handle>(base);
    return UACPI_STATUS_OK;
};

void uacpi_kernel_io_unmap(const uacpi_handle) {
    // No-op
}

uacpi_status uacpi_kernel_io_read8(
    const uacpi_handle handle, const uacpi_size offset, uacpi_u8* out_value
) {
    *out_value = inb(static_cast<uint16_t>(reinterpret_cast<uintptr_t>(handle) + offset));
    return UACPI_STATUS_OK;
};

uacpi_status uacpi_kernel_io_read16(
    const uacpi_handle handle, const uacpi_size offset, uacpi_u16* out_value
) {
    *out_value = inw(static_cast<uint16_t>(reinterpret_cast<uintptr_t>(handle) + offset));
    return UACPI_STATUS_OK;
};

uacpi_status uacpi_kernel_io_read32(
    const uacpi_handle handle, const uacpi_size offset, uacpi_u32* out_value
) {
    *out_value = inl(static_cast<uint16_t>(reinterpret_cast<uintptr_t>(handle) + offset));
    return UACPI_STATUS_OK;
};

uacpi_status uacpi_kernel_io_write8(const uacpi_handle handle, const uacpi_size offset, const uacpi_u8 in_value) {
    outb(in_value, static_cast<uint16_t>(reinterpret_cast<uintptr_t>(handle) + offset));
    return UACPI_STATUS_OK;
};

uacpi_status uacpi_kernel_io_write16(const uacpi_handle handle, const uacpi_size offset, const uacpi_u16 in_value) {
    outw(in_value, static_cast<uint16_t>(reinterpret_cast<uintptr_t>(handle) + offset));
    return UACPI_STATUS_OK;
};

uacpi_status uacpi_kernel_io_write32(const uacpi_handle handle, const uacpi_size offset, const uacpi_u32 in_value) {
    outl(in_value, static_cast<uint16_t>(reinterpret_cast<uintptr_t>(handle) + offset));
    return UACPI_STATUS_OK;
};

void* uacpi_kernel_alloc(const uacpi_size size) {
    return kalloc(size);
}

void uacpi_kernel_free(void* mem) {
    kfree(mem);
}

uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot() {
    return timer::ns_since_boot();
};

void uacpi_kernel_stall(uacpi_u8 usec) {
    auto now = timer::ns_since_boot();
    auto timeout = now + (usec * 1000ULL);

    while (timer::ns_since_boot() < timeout) {
        asm volatile("pause");
    }
}

/**
 * Sleep for N milliseconds.
 */
void uacpi_kernel_sleep(const uacpi_u64 msec) {
    timer::sleep_ms(msec);
}

/**
 * Create/free an opaque non-recursive kernel mutex object.
 */
uacpi_handle uacpi_kernel_create_mutex() {
    const auto m = new mutex();
    return m;
}

void uacpi_kernel_free_mutex(const uacpi_handle handle) {
    if (!handle) return;
    const auto m = static_cast<mutex*>(handle);
    delete m;
}

/**
 * Create/free an opaque kernel (semaphore-like) event object.
 */
uacpi_handle uacpi_kernel_create_event() {
    return new acpi::event();
}

void uacpi_kernel_free_event(const uacpi_handle handle) {
    delete static_cast<acpi::event*>(handle);
}

uacpi_thread_id uacpi_kernel_get_thread_id() {
    return 0; // TODO: implement thread id
}

uacpi_status uacpi_kernel_acquire_mutex(const uacpi_handle handle, const uacpi_u16 timeout) {
    if (!handle) return UACPI_STATUS_INVALID_ARGUMENT;
    // const auto m = static_cast<mutex*>(handle);
    //
    // if (timeout == 0xFFFF) {
    //     m->acquire();
    //     return UACPI_STATUS_OK;
    // }
    //
    // if (m->try_lock_for(std::chrono::milliseconds(timeout))) {
    //     return UACPI_STATUS_OK;
    // }

    return UACPI_STATUS_TIMEOUT;
}

void uacpi_kernel_release_mutex(const uacpi_handle handle) {
    if (!handle) return;
    const auto m = static_cast<mutex*>(handle);

    m->release();
}

/**
 * Try to wait for an event (counter > 0) with a millisecond timeout.
 * A timeout value of 0xFFFF implies infinite wait.
 *
 * The internal counter is decremented by 1 if wait was successful.
 *
 * A successful wait is indicated by returning UACPI_TRUE.
 */
uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle handle, uacpi_u16 timeout) {
    return UACPI_FALSE; // TODO: implement event waiting
};

/**
 * Signal the event object by incrementing its internal counter by 1.
 *
 * This function may be used in interrupt contexts.
 */
void uacpi_kernel_signal_event(const uacpi_handle handle) {
    const auto e = static_cast<acpi::event*>(handle);
};

/**
 * Reset the event counter to 0.
 */
void uacpi_kernel_reset_event(const uacpi_handle handle) {
    const auto e = static_cast<acpi::event*>(handle);

    e->reset();
};

/**
 * Handle a firmware request.
 *
 * Currently either a Breakpoint or Fatal operators.
 */
uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request*) {
    return UACPI_STATUS_OK; // TODO: implement firmware request handling
}

/**
 * Install an interrupt handler at 'irq', 'ctx' is passed to the provided
 * handler for every invocation.
 *
 * 'out_irq_handle' is set to a kernel-implemented value that can be used to
 * refer to this handler from other API.
 */
uacpi_status uacpi_kernel_install_interrupt_handler(
    const uacpi_u32 irq, uacpi_interrupt_handler handler, uacpi_handle ctx,
    uacpi_handle* out_irq_handle
) {
    return UACPI_STATUS_OK; // TODO: implement interrupt handler installation
};

/**
 * Uninstall an interrupt handler. 'irq_handle' is the value returned via
 * 'out_irq_handle' during installation.
 */
uacpi_status uacpi_kernel_uninstall_interrupt_handler(
    uacpi_interrupt_handler handler, const uacpi_handle irq_handle
) {
    return UACPI_STATUS_OK; // TODO: implement interrupt handler uninstallation
};

/**
 * Create/free a kernel spinlock object.
 *
 * Unlike other types of locks, spinlocks may be used in interrupt contexts.
 */
uacpi_handle uacpi_kernel_create_spinlock() {
    return new kernel::spinlock();
};

void uacpi_kernel_free_spinlock(const uacpi_handle handle) {
    if (!handle) return;
    const auto m = static_cast<kernel::spinlock*>(handle);
    delete m;
};

/**
 * Lock/unlock helpers for spinlocks.
 *
 * These are expected to disable interrupts, returning the previous state of cpu
 * flags, that can be used to possibly re-enable interrupts if they were enabled
 * before.
 *
 * Note that lock is infalliable.
 */
uacpi_cpu_flags uacpi_kernel_lock_spinlock(const uacpi_handle handle) {
    const auto m = static_cast<kernel::spinlock*>(handle);
    m->lock();
    return 0; // TODO: implement cpu flags
}

void uacpi_kernel_unlock_spinlock(const uacpi_handle handle, uacpi_cpu_flags cpu_flags) {
    const auto m = static_cast<kernel::spinlock*>(handle);
    m->unlock();
}

/*
 * Schedules deferred work for execution.
 * Might be invoked from an interrupt context.
 */
uacpi_status uacpi_kernel_schedule_work(
    const uacpi_work_type type, const uacpi_work_handler handler, uacpi_handle ctx
) {
    // TODO: implement work scheduling
    switch (type) {
        case UACPI_WORK_GPE_EXECUTION:
            break;
        case UACPI_WORK_NOTIFICATION:
            break;
        default:
            return UACPI_STATUS_INVALID_ARGUMENT;
    }

    return UACPI_STATUS_OK;
};

/*
 * Waits for two types of work to finish:
 * 1. All in-flight interrupts installed via uacpi_kernel_install_interrupt_handler
 * 2. All work scheduled via uacpi_kernel_schedule_work
 *
 * Note that the waits must be done in this order specifically.
 */
uacpi_status uacpi_kernel_wait_for_work_completion() {
    return UACPI_STATUS_OK; // TODO: implement work completion waiting
}

#endif

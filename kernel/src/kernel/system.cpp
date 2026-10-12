#include <kernel/log.hpp>
#include <kernel/system.hpp>
#include <kernel/debug/debug.hpp>

// read from b
uint8_t inb(uint16_t port) {
    uint8_t rv;
    asm volatile("inb %1, %0" : "=a" (rv) : "dN" (port));
    return rv;
}

uint16_t inw(uint16_t port) {
    uint16_t rv;
    asm volatile("inw %1, %0" : "=a" (rv) : "dN" (port));
    return rv;
}

uint32_t inl(uint16_t port) {
    uint32_t rv;
    asm volatile("inl %1, %0" : "=a" (rv) : "dN" (port));
    return rv;
}

void outb(uint16_t port, uint8_t data) {
    asm volatile("outb %1, %0" : : "dN" (port), "a" (data));
}

void outw(uint16_t port, uint16_t data) {
    asm volatile("outw %1, %0" : : "dN" (port), "a" (data));
}

void outl(uint16_t port, uint32_t data) {
    asm volatile("outl %1, %0" : : "dN" (port), "a" (data));
}

/**
 * Trigger a kernel panic, logging the message and halting the system.
 * @param msg
 * @param ...
 */
void panic(const char* msg, ...) {
    va_list ap;
    va_start(ap, msg);
    logger.vlog(LOG_LEVEL_FATAL, msg, ap);
    va_end(ap);

    debug::print_stacktrace(20);

    for (;;) asm volatile("hlt");
}

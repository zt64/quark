#include "kernel/exception.hpp"

#include <kernel/process.hpp>

#include "kernel/log.hpp"
#include "kernel/system.hpp"

namespace {
    constexpr const char* exception_messages[] = {
        "Divide By Zero", "Debug", "Non-maskable Interrupt", "Breakpoint",
        "Overflow", "Bound Range Exceeded", "Invalid Opcode", "Device Not Available",
        "Double Fault", "Coprocessor Segment Overrun", "Invalid TSS", "Segment Not Present",
        "Stack-Segment Fault", "General Protection Fault", "Page Fault", "Reserved",
        "x87 Floating-Point Exception", "Alignment Check", "Machine Check",
        "SIMD Floating-Point Exception", "Virtualization Exception",
        "Control Protection Exception", "Reserved", "Reserved", "Reserved", "Reserved",
        "Reserved", "Reserved", "Reserved", "Hypervisor Injection Exception",
        "VMM Communication Exception", "Security Exception", "Reserved"
    };
}

extern "C" [[noreturn]] void fault_handler(const regs* r) {
    logger.fatal("%s Exception. System Halted.", exception_messages[r->int_no]);
    logger.fatal("Registers:");
    logger.fatal("RAX=0x%016lx RBX=0x%016lx RCX=0x%016lx RDX=0x%016lx", r->rax, r->rbx, r->rcx, r->rdx);
    logger.fatal("RSI=0x%016lx RDI=0x%016lx RBP=0x%016lx", r->rsi, r->rdi, r->rbp);
    logger.fatal("R8 =0x%016lx R9 =0x%016lx R10=0x%016lx R11=0x%016lx", r->r8, r->r9, r->r10, r->r11);
    logger.fatal("R12=0x%016lx R13=0x%016lx R14=0x%016lx R15=0x%016lx", r->r12, r->r13, r->r14, r->r15);
    logger.fatal("RIP=0x%016lx RSP=0x%016lx RFLAGS=0x%016lx", r->rip, r->rsp, r->rflags);
    logger.fatal("CS=0x%04lx SS=0x%04lx", r->cs, r->ss);
    logger.fatal("Int#=0x%02lx Err=0x%016lx", r->int_no, r->err_code);

    uint64_t cr2;
    asm volatile("mov %%cr2, %0" : "=r"(cr2));
    logger.fatal("CR2=0x%016lx", cr2);

    if (r->cs & 0x11) {
        logger.fatal("Faulting address is in user space. Terminating offending process.");

        // TODO: Send signal to task
        terminate_task(current);
    } else {
        logger.fatal("Faulting address is in kernel space. Halting.");
        asm volatile("cli; hlt");
        for (;;) {}
    }
}

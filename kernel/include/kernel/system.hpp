#pragma once

#include <cstdint>

struct regs {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no;
    uint64_t err_code;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

uint8_t inb(uint16_t port);

void outb(uint16_t port, uint8_t data);

uint32_t inl(uint16_t port);

void outl(uint16_t port, uint32_t data);

[[noreturn]] void panic(const char* msg, ...);
BITS 64

global _start
extern main

_start:
    xor ebp, ebp
    and rsp, -16
    call main

.halt:
    hlt
    jmp .halt

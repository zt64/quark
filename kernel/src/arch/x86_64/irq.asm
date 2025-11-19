BITS 64

%assign i 0
%rep 16
    global irq_stub_%+i
    irq_stub_%+i:
        cli
        push 0
        push i + 32
        jmp irq_common_stub
%assign i i+1
%endrep

global irq_stub_table
irq_stub_table:
%assign i 0
%rep 16
    dq irq_stub_%+i
%assign i i+1
%endrep

extern irq_handle
; https://github.com/i3vie/neutrino/blob/master/src/arch/x86_64/isr_stubs.S
irq_common_stub:
    cli
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp ; the stack coincidentally is shaped like a C struct so we can just throw that into C
    call irq_handle

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16
    iretq
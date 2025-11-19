BITS 64

%macro isr_no_err_stub 1
global isr_stub_%+%1
isr_stub_%+%1:
    push 0
    push %1
    jmp isr_common_stub
    iret
%endmacro

%macro isr_err_stub 1
global isr_stub_%+%1
isr_stub_%+%1:
    push %1
    jmp isr_common_stub
%endmacro

extern current
extern isr_dispatch      ; was: extern fault_handler
%define TASK_ESP 8
%define TASK_TRAP_FRAME 48

isr_common_stub:
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

    ; Save the current interrupt frame as the task's kernel context.
    mov rax, [rel current]
    test rax, rax
    jz .dispatch

    mov [rax + TASK_TRAP_FRAME], rsp

.dispatch:
    mov rdi, rsp
    call isr_dispatch

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

global isr_stub_128
isr_stub_128:
    push 0
    push 0x80
    jmp isr_common_stub

isr_no_err_stub 0
isr_no_err_stub 1
isr_no_err_stub 2
isr_no_err_stub 3
isr_no_err_stub 4
isr_no_err_stub 5
isr_no_err_stub 6
isr_no_err_stub 7
isr_err_stub    8
isr_no_err_stub 9
isr_err_stub    10
isr_err_stub    11
isr_err_stub    12
isr_err_stub    13
isr_err_stub    14
isr_no_err_stub 15
isr_no_err_stub 16
isr_err_stub    17
isr_no_err_stub 18
isr_no_err_stub 19
isr_no_err_stub 20
isr_no_err_stub 21
isr_no_err_stub 22
isr_no_err_stub 23
isr_no_err_stub 24
isr_no_err_stub 25
isr_no_err_stub 26
isr_no_err_stub 27
isr_no_err_stub 28
isr_no_err_stub 29
isr_err_stub    30
isr_no_err_stub 31

global isr_stub_table
isr_stub_table:
%assign i 0
%rep    32
    dq isr_stub_%+i ; use DQ instead if targeting 64-bit
%assign i i+1
%endrep

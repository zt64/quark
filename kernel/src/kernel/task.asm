BITS 64

extern current
extern tss_entry

%define TASK_RSP   8
%define TASK_RSP0  16
%define TASK_CR3   24

%define TSS_RSP0   4

global switch_to_task
switch_to_task:
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    mov rax, [rel current]
    test rax, rax
    jz .load_next

    mov [rax + TASK_RSP], rsp

.load_next:
    mov [rel current], rdi

    mov rsp, [rdi + TASK_RSP]

    mov rax, cr3
    mov rcx, [rdi + TASK_CR3]

    cmp rax, rcx
    je .same_cr3

    mov cr3, rcx

.same_cr3:
    mov rax, [rdi + TASK_RSP0]
    mov [rel tss_entry + TSS_RSP0], rax

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    ret
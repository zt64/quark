BITS 64

; rdi = prev_rsp_slot (void**, may be null to skip saving the old rsp)
; rsi = next_rsp      (value to load into rsp)
; rdx = next_cr3      (value to load into cr3, if different from current)
global raw_switch_to_task
raw_switch_to_task:
    push rax
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    test rdi, rdi
    jz .load_next
    mov [rdi], rsp
.load_next:
    mov rsp, rsi

    mov rax, cr3
    cmp rax, rdx
    je .same_cr3

    mov cr3, rdx
.same_cr3:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    pop rax
    ret

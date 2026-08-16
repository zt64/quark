section .text
global _start
extern __mlibc_entry
extern main

_start:
    xor rbp, rbp
    mov rdi, rsp        ; entry_stack — raw pointer to argc/argv/envp block on the stack
    lea rsi, [rel main] ; main_fn — address of the program's main()
    and rsp, -16        ; align stack per SysV ABI before the call
    call __mlibc_entry
    hlt                 ; unreachable — __mlibc_entry calls exit(), never returns
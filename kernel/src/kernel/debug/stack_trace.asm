BITS 64

; Walks backwards through the call stack and builds a list of return addresses.
; Args:
;  * Array of 32-bit addresses.
;  * Maximum number of elements in array.
; Return value: The number of addresses stored in the array.
; Calling convention: cdecl
[global walk_stack]
walk_stack:
    ; Create stack frame & save caller's EDI and EBX.
    push rbp
    mov  rbp,       rsp
    sub  rsp,       8
    mov  [rbp - 4], edi
    mov  [rbp - 8], ebx
    ; Set up local registers.
    xor  rax,       rax         ; RAX = return value (number of stack frames found).
    mov  rbx,       [rsp +  8]  ; RBX = old RBP.
    mov  rdi,       [rsp + 16]  ; Destination array pointer in RDI.
    mov  rcx,       [rsp + 20]  ; Maximum array size in RCX.
.walk:
    ; Walk backwards through RBP linked list, storing return addresses in RDI array.
    test rbx,       rbx
    jz   .done
    mov  rdx,       [rbx +  4]  ; RDX = previous stack frame's IP.
    mov  rbx,       [rbx +  0]  ; RBX = previous stack frame's BP.
    mov  [rdi],     rdx         ; Copy IP.
    add  rdi,       8
    inc  rax
    loop .walk
.done:
    ; Restore caller's EDI and EBX, leave stack frame & return EAX.
    mov  edi,       [rbp - 4]
    mov  ebx,       [rbp - 8]
    leave
    ret
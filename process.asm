; ZevOS kernel context switch
BITS 64

section .text
global process_switch

; void process_switch(uint64_t *old_rsp, uint64_t new_rsp)
; Save the SysV callee-saved registers and switch to another kernel stack.
process_switch:
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov [rdi], rsp
    mov rsp, rsi

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

section .note.GNU-stack noalloc noexec nowrite progbits

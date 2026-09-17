BITS 64
section .text
global syscall_entry
extern syscall_dispatch

syscall_entry:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; CPU's ring-3 interrupt frame leaves RSP misaligned for SysV calls.
    ; Add 8 bytes so syscall_dispatch enters with the required alignment.
    sub rsp, 8

    ; syscall_dispatch(number, arg1, arg2, arg3)
    mov rcx, rdx
    mov rdx, rsi
    mov rsi, rdi
    mov rdi, rax
    call syscall_dispatch

    add rsp, 8
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    iretq

section .note.GNU-stack noalloc noexec nowrite progbits

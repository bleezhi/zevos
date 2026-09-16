BITS 64
section .text
global syscall_entry
extern syscall_dispatch

syscall_entry:
    ; Userspace ABI:
    ; RAX = syscall number, RDI/RSI/RDX = arguments.
    ; Preserve the callee-saved registers used by the kernel ABI.
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; Rearrange registers for syscall_dispatch(number, arg1, arg2).
    mov rdx, rsi
    mov rsi, rdi
    mov rdi, rax
    call syscall_dispatch

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    iretq

section .note.GNU-stack noalloc noexec nowrite progbits

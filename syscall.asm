BITS 64
section .text
global syscall_entry
extern syscall_dispatch

syscall_entry:
    /* Early syscall ABI foundation: RAX is the syscall number.
     * Arguments and full register preservation will be added with
     * the real userspace process ABI.
     */
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov rdi, rax
    call syscall_dispatch
    push rax

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    pop rax
    iretq

section .note.GNU-stack noalloc noexec nowrite progbits

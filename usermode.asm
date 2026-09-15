; ZevOS ring 3 transition helper
BITS 64

section .text
global user_enter

; void user_enter(entry, stack, user_cs, user_ss)
user_enter:
    mov ax, dx
    mov ds, ax
    mov es, ax

    ; Build the frame consumed by iretq:
    ; SS, RSP, RFLAGS, CS, RIP.
    push rcx
    push rsi
    pushfq
    pop rax
    or rax, 0x200
    push rax
    push rdx
    push rdi
    iretq

section .note.GNU-stack noalloc noexec nowrite progbits

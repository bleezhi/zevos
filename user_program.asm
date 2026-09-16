BITS 64
section .userprog
align 16
global user_program_start
global user_program_end
%define USER_BASE 0x400000
%define ENTRY 0x400078
%define MESSAGE 0x400100
user_program_start:
    db 0x7F, 'E', 'L', 'F'
    db 2, 1, 1, 0
    times 8 db 0
    dw 2
    dw 0x3E
    dd 1
    dq ENTRY
    dq 64
    dq 0
    dd 0
    dw 64
    dw 56
    dw 1
    dw 0
    dw 0
    dw 0
    dd 1
    dd 5
    dq 0
    dq USER_BASE
    dq USER_BASE
    dq user_program_end - user_program_start
    dq user_program_end - user_program_start
    dq 0x1000
entry_point:
    mov rdi, 1
    mov rsi, MESSAGE
    mov rdx, 24
    mov rax, 1
    int 0x80
    xor rdi, rdi
    mov rax, 3
    int 0x80
.hang:
    hlt
    jmp .hang
times MESSAGE - USER_BASE - ($ - user_program_start) db 0
user_message:
    db "Hello from ZevOS zinit!", 10
user_program_end:
section .note.GNU-stack noalloc noexec nowrite progbits

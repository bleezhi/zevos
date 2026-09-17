BITS 64
section .userprog
align 16
global user_program_start
global user_program_entry
global user_program_end

%define USER_BASE 0x400000
%define ENTRY 0x400078
%define MESSAGE 0x400140
%define PATH    0x400160
%define BUFFER  0x400180

; First tiny ELF userspace program. It deliberately exercises the complete
; bootstrap syscall set before exiting: write, open, read, write, close, exit.
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

    ; One PT_LOAD segment covering this entire bootstrap image.
    dd 1
    dd 7
    dq 0
    dq USER_BASE
    dq USER_BASE
    dq user_program_end - user_program_start
    dq user_program_end - user_program_start
    dq 0x1000

user_program_entry:
    ; sys_write(1, MESSAGE, 24)
    mov rdi, 1
    mov rsi, MESSAGE
    mov rdx, 24
    mov rax, 1
    int 0x80

    ; sys_open(PATH, 0)
    mov rdi, PATH
    xor rsi, rsi
    mov rax, 2
    int 0x80
    test rax, rax
    js .exit_error
    mov r12, rax

    ; sys_read(fd, BUFFER, 64)
    mov rdi, r12
    mov rsi, BUFFER
    mov rdx, 64
    mov rax, 0
    int 0x80
    test rax, rax
    js .close_error

    ; sys_write(1, BUFFER, bytes_read)
    mov rdx, rax
    mov rdi, 1
    mov rsi, BUFFER
    mov rax, 1
    int 0x80

    ; sys_close(fd)
    mov rdi, r12
    mov rax, 4
    int 0x80
    test rax, rax
    js .exit_error

    ; sys_exit(0)
    xor rdi, rdi
    mov rax, 3
    int 0x80

.exit_error:
    mov rdi, 1
    mov rax, 3
    int 0x80

.close_error:
    mov rdi, r12
    mov rax, 4
    int 0x80
    jmp .exit_error

; Keep fixed virtual-address data inside the same mapped PT_LOAD image.
times MESSAGE - USER_BASE - ($ - user_program_start) db 0
user_message:
    db "Hello from ZevOS zinit!", 10

times PATH - USER_BASE - ($ - user_program_start) db 0
user_path:
    db "/etc/hostname", 0

times BUFFER - USER_BASE - ($ - user_program_start) db 0
user_buffer:
    times 64 db 0

user_program_end:
section .note.GNU-stack noalloc noexec nowrite progbits

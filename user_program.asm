BITS 64

; Tiny first ZevOS userspace program.
; It is position-independent only by virtue of using fixed virtual addresses.
; The kernel copies this blob to user virtual address 0x400000.

section .userprog
align 16
global user_program_start
global user_program_end

user_program_start:
    ; syscall 1 = write console string
    mov rdi, 0x400100
    mov rax, 1
    int 0x80

    ; Keep the first userspace task alive until a real exit syscall exists.
.hang:
    jmp .hang

times 0x100 - ($ - user_program_start) db 0

user_message:
    db "Hello from ZevOS userspace!", 10, 0

user_program_end:

section .note.GNU-stack noalloc noexec nowrite progbits

; Stage 0 placeholder for the userspace cat program.
; The syscall/VFS implementation will replace this with a real
; argument-aware implementation once argv is available.
BITS 64
org 0x400000

_start:
    mov eax, 3
    xor edi, edi
    int 0x80

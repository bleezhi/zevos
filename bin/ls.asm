; Stage 0 placeholder for userspace ls.
; Directory enumeration will be wired through a syscall before
; this becomes a full implementation.
BITS 64
org 0x400000

_start:
    mov eax, 3
    xor edi, edi
    int 0x80

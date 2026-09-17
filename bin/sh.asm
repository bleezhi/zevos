; ZevOS userspace shell — Stage 0 source placeholder.
; The kernel shell remains temporarily available while argv/stdin and
; process waiting are migrated to userspace.
BITS 64
org 0x400000

_start:
    mov eax, 3
    xor edi, edi
    int 0x80

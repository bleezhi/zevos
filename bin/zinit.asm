; ZevOS userspace zinit (PID 1) — Stage 0 bootstrap.
BITS 64
org 0x400000

_start:
    mov eax, 1
    mov edi, 1
    mov rsi, msg
    mov edx, msg_end-msg
    int 0x80
    mov eax, 3
    xor edi, edi
    int 0x80

msg db 'zinit: userspace PID 1',10
msg_end:

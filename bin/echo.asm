; ZevOS /bin/echo
; Stage 0 userspace source.
BITS 64
org 0x400000

elf:
    db 0x7f, 'E','L','F', 2,1,1,0
    times 8 db 0
    dw 2, 0x3e
    dd 1
    dq _start
    dq phdr - $$
    dq 0
    dd 0
    dw 64, 56, 1, 0, 0, 0

phdr:
    dd 1, 5
    dq 0
    dq $$
    dq $$
    dq elf_end - $$
    dq elf_end - $$
    dq 0x1000

_start:
    mov eax, 1
    mov edi, 1
    mov rsi, message
    mov edx, message_end - message
    int 0x80
    mov eax, 3
    xor edi, edi
    int 0x80

message db 'echo: ZevOS', 10
message_end:
elf_end:

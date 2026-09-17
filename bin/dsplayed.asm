; dsplayed — Stage 0 userspace display server bootstrap.
; This must stay a valid ELF64 executable. It currently only proves
; that the display server can be launched as an independent process.
BITS 64
org 0x400000

_start:
    mov eax, 1
    mov edi, 1
    mov rsi, msg
    mov edx, msg_end-msg
    int 0x80

    ; Stay alive: dsplayed is a service, not a one-shot command.
.loop:
    pause
    jmp .loop

msg db 'dsplayed: stage 0',10
msg_end:

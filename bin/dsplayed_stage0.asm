; Valid ELF64 Stage 0 dsplayed image source.
BITS 64
org 0x400000
elf:
 db 0x7f,'ELF',2,1,1,0
 times 8 db 0
 dw 2,0x3e
 dd 1
 dq _start
 dq phdr-$$
 dq 0
 dd 0
 dw 64,56,1,0,0,0
phdr:
 dd 1,5
 dq 0
 dq $$
 dq $$
 dq end-$$
 dq end-$$
 dq 0x1000
_start:
 mov eax,1
 mov edi,1
 mov rsi,msg
 mov edx,msg_end-msg
 int 0x80
.loop:
 pause
 jmp .loop
msg db 'dsplayed: stage 0',10
msg_end:
end:

BITS 64
org 0x400000

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
 mov eax,3
 xor edi,edi
 int 0x80
end:

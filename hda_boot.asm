BITS 16
ORG 0x7C00
start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    mov [boot_drive], dl
    mov word [dap.count], 1
    mov word [dap.offset], 0x8000
    mov word [dap.segment], 0
    mov dword [dap.lba_lo], 1
    mov dword [dap.lba_hi], 0
    mov si, dap
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jc error
    cmp dword [0x8000], 0x544F425A
    jne error
    mov eax, [0x8004]
    test eax, eax
    jz error
    mov [remaining], eax
    mov dword [current_lba], 2
    mov dword [dest_lo], 0x00100000
.load:
    mov eax, [remaining]
    cmp eax, 127
    jbe .count
    mov eax, 127
.count:
    mov [dap.count], ax
    mov eax, [current_lba]
    mov [dap.lba_lo], eax
    mov eax, [dest_lo]
    mov [dap.buffer_lo], eax
    mov dword [dap.buffer_hi], 0
    mov si, dap
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jc error
    movzx eax, word [dap.count]
    sub [remaining], eax
    add [current_lba], eax
    shl eax, 9
    add [dest_lo], eax
    cmp dword [remaining], 0
    jne .load
    in al, 0x92
    or al, 2
    out 0x92, al
    lgdt [gdt_ptr]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:protected
BITS 32
protected:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x90000
    mov eax, 0x36d76289
    xor ebx, ebx
    jmp 0x00100000
BITS 16
error:
    mov si, msg
.print:
    lodsb
    test al, al
    jz .halt
    mov ah, 0x0e
    int 0x10
    jmp .print
.halt:
    cli
    hlt
    jmp .halt
boot_drive db 0
remaining dd 0
current_lba dd 0
dest_lo dd 0
msg db 'ZevOS HDA boot error',0
align 4
gdt:
    dq 0
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt_ptr:
    dw gdt_ptr-gdt-1
    dd gdt
align 4
dap:
    db 0x18,0
.count: dw 0
.offset: dw 0
.segment: dw 0
.buffer_lo: dd 0
.buffer_hi: dd 0
.lba_lo: dd 0
.lba_hi: dd 0
times 510-($-$$) db 0
dw 0xAA55

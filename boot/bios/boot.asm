; ZevBoot BIOS stage 1.
; Loads the fixed-size ZevBoot BIOS stage 2 from LBA 2 into 0x8000.
BITS 16
ORG 0x7C00

; El Torito Boot Info Table is patched by xorriso at offset 8.
; Offset 12 contains the LBA of this boot image. For HDA it remains zero.
times 8-($-$) db 0
boot_info_table:
    dd 0
    dd 0
    dd 0
    dd 0
    times 40 db 0

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    mov [boot_drive], dl

    mov eax, [boot_info_table + 4]
    mov [0x7BF0], eax

    mov word [dap.count], 16
    mov word [dap.offset], 0x8000
    mov word [dap.segment], 0
    mov dword [dap.buffer_lo], 0x00008000
    mov dword [dap.buffer_hi], 0
    mov eax, [0x7BF0]
    add eax, 2
    mov [dap.lba_lo], eax
    mov dword [dap.lba_hi], 0

    mov si, dap
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    jc disk_error

    jmp 0x0000:0x8000

disk_error:
    mov si, msg
.print:
    lodsb
    test al, al
    jz .halt
    mov ah, 0x0E
    int 0x10
    jmp .print
.halt:
    cli
    hlt
    jmp .halt

boot_drive db 0
msg db 'ZevBoot BIOS: stage 2 read failed',0

align 4
dap:
    db 0x10, 0
.count: dw 0
.offset: dw 0
.segment: dw 0
.buffer_lo: dd 0
.buffer_hi: dd 0
.lba_lo: dd 0
.lba_hi: dd 0

times 510-($-$$) db 0
dw 0xAA55

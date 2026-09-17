BITS 16
ORG 0x7C00

; BIOS loads this sector at 0000:7C00.  Keep all BIOS disk I/O below 1 MiB;
; some BIOS/firmware implementations reject EDD transfers whose destination
; is at 0x100000.  We stage the kernel at 0x10000, then copy it upward after
; entering protected mode.
start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    mov [boot_drive], dl

    ; Read the ZBOT header from LBA 1.
    mov word [dap.count], 1
    mov word [dap.offset], 0x8000
    mov word [dap.segment], 0
    mov dword [dap.buffer_lo], 0x00008000
    mov dword [dap.buffer_hi], 0
    mov dword [dap.lba_lo], 1
    mov dword [dap.lba_hi], 0
    call edd_read
    jc header_error

    cmp dword [0x8000], 0x544F425A       ; "ZBOT"
    jne header_error

    mov eax, [0x8004]                    ; kernel sector count
    test eax, eax
    jz header_error

    ; Staging area is 0x10000..0xEFFFF (896 KiB).
    ; The current kernel must fit here before it is copied to 1 MiB.
    cmp eax, 1792
    ja kernel_too_large
    mov [remaining], eax
    mov dword [current_lba], 2
    mov dword [stage_lo], 0x00010000

.load:
    mov eax, [remaining]
    cmp eax, 127
    jbe .count
    mov eax, 127
.count:
    mov [dap.count], ax
    mov eax, [current_lba]
    mov [dap.lba_lo], eax
    mov eax, [stage_lo]
    mov [dap.buffer_lo], eax
    mov dword [dap.buffer_hi], 0
    call edd_read
    jc disk_error

    movzx eax, word [dap.count]
    sub [remaining], eax
    add [current_lba], eax
    shl eax, 9
    add [stage_lo], eax
    cmp dword [remaining], 0
    jne .load

    ; Enable A20 and enter 32-bit protected mode.
    in al, 0x92
    or al, 2
    out 0x92, al

    lgdt [gdt_ptr]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:protected

; EDD read using the BIOS INT 13h extensions.
; DS:SI points at our 0x18-byte DAP.
BITS 16
edd_read:
    mov si, dap
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    ret

BITS 32
protected:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x90000

    ; Copy the staged kernel from 0x10000 to its linked address 0x100000.
    ; The kernel image contains only file-backed sections; boot.asm clears BSS.
    mov esi, 0x00010000
    mov edi, 0x00100000
    mov ecx, [kernel_sectors]
    shl ecx, 7                       ; sectors * 512 / 4 = sectors * 128
    rep movsd

    ; boot.asm expects the Multiboot2 magic in EAX and a null info pointer.
    mov eax, 0x36d76289
    xor ebx, ebx
    jmp 0x00100000

BITS 16
header_error:
    mov si, msg_header
    jmp print_error

disk_error:
    mov si, msg_disk
    jmp print_error

kernel_too_large:
    mov si, msg_large
    jmp print_error

print_error:
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
stage_lo dd 0
kernel_sectors dd 0
msg_header db 'ZevOS HDA: invalid kernel header',0
msg_disk   db 'ZevOS HDA: disk read failed',0
msg_large  db 'ZevOS HDA: kernel too large',0

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

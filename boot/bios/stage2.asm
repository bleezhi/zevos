; ZevBoot BIOS stage 2.
; Layout in the boot image:
;   LBA 0  stage 1
;   LBA 1  ZevBoot header
;   LBA 2-17 stage 2 (16 sectors)
;   LBA 18+ kernel
BITS 16
ORG 0x8000

%define BOOT_INFO 0x5000
%define E820_MAP  0x6000
%define E820_COUNT 0x5FFC
%define HEADER    0x7000
%define KERNEL_STAGE 0x10000
%define BOOT_IMAGE_LBA_PTR 0x7BF0
%define STAGE2_SECTORS 16
%define MAX_KERNEL_SECTORS 1792

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    mov [boot_drive], dl
    mov eax, [BOOT_IMAGE_LBA_PTR]
    mov [boot_image_lba], eax

    ; Read ZevBoot header from the sector immediately after stage 1.
    mov word [dap.count], 1
    mov word [dap.offset], HEADER
    mov word [dap.segment], 0
    mov dword [dap.buffer_lo], HEADER
    mov dword [dap.buffer_hi], 0
    mov eax, [boot_image_lba]
    add eax, 1
    mov [dap.lba_lo], eax
    mov dword [dap.lba_hi], 0
    call edd_read
    jc header_error

    cmp dword [HEADER], 0x544F425A          ; ZBOT
    jne header_error
    mov eax, [HEADER+4]
    test eax, eax
    jz header_error
    cmp eax, MAX_KERNEL_SECTORS
    ja kernel_too_large
    mov [kernel_sectors], eax
    mov eax, [HEADER+8]
    mov [kernel_bytes], eax

    ; Collect the BIOS E820 memory map.
    xor ebx, ebx
    mov di, E820_MAP
    mov word [E820_COUNT], 0
.e820:
    mov eax, 0xE820
    mov edx, 0x534D4150
    mov ecx, 24
    int 0x15
    jc .done
    cmp eax, 0x534D4150
    jne .done
    cmp ecx, 20
    jb .next
    ; Keep the complete 24-byte E820 entry.
    add di, 24
    inc word [E820_COUNT]
    cmp word [E820_COUNT], 64
    jae .done
.next:
    test ebx, ebx
    jnz .e820
.done:
    cmp word [E820_COUNT], 0
    jne .load_kernel
    jmp e820_error

.load_kernel:
    mov eax, [kernel_sectors]
    mov [remaining], eax
    mov eax, [HEADER+12]
    test eax, eax
    jz header_error
    add eax, [boot_image_lba]
    mov [current_lba], eax
    mov dword [stage_lo], KERNEL_STAGE

.read_loop:
    mov eax, [remaining]
    cmp eax, 32
    jbe .count
    mov eax, 32
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
    jne .read_loop

    ; Build the ZevBootInfo structure at 0x5000.
    ; struct zev_boot_info:
    ; +0 magic, +8 version, +16 map, +24 entries, +32 entry size
    ; +40 framebuffer, +48 width, +52 height, +56 pitch, +60 format
    ; +64 kernel base, +72 kernel end, +80 boot drive, +88 flags.
    mov dword [BOOT_INFO+0], 0x4F4F5446       ; low half of ZEVBOOT magic
    mov dword [BOOT_INFO+4], 0x5A455642       ; high half
    mov dword [BOOT_INFO+8], 1
    mov dword [BOOT_INFO+12], 0
    mov dword [BOOT_INFO+16], E820_MAP
    mov dword [BOOT_INFO+20], 0
    movzx eax, word [E820_COUNT]
    mov dword [BOOT_INFO+24], eax
    mov dword [BOOT_INFO+28], 0
    mov dword [BOOT_INFO+32], 24
    mov dword [BOOT_INFO+36], 0
    ; No framebuffer on legacy BIOS stage 0.
    mov dword [BOOT_INFO+40], 0
    mov dword [BOOT_INFO+44], 0
    mov dword [BOOT_INFO+48], 0
    mov dword [BOOT_INFO+52], 0
    mov dword [BOOT_INFO+56], 0
    mov dword [BOOT_INFO+60], 0
    mov dword [BOOT_INFO+64], 0x00100000
    mov dword [BOOT_INFO+68], 0
    mov eax, [kernel_bytes]
    add eax, 0x00100000
    mov dword [BOOT_INFO+72], eax
    mov dword [BOOT_INFO+76], 0
    movzx eax, byte [boot_drive]
    mov dword [BOOT_INFO+80], eax
    mov dword [BOOT_INFO+84], 0
    mov dword [BOOT_INFO+88], 1
    mov dword [BOOT_INFO+92], 0

    ; Enable A20 and enter protected mode.
    in al, 0x92
    or al, 2
    out 0x92, al

    lgdt [gdt_ptr]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:protected

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

    ; Copy staged kernel to its linked physical address.
    mov esi, KERNEL_STAGE
    mov edi, 0x00100000
    mov ecx, [kernel_sectors]
    shl ecx, 7
    rep movsd

    mov ebx, BOOT_INFO
    jmp 0x00100000

header_error:
    mov si, msg_header
    jmp print_error
disk_error:
    mov si, msg_disk
    jmp print_error
kernel_too_large:
    mov si, msg_large
    jmp print_error
e820_error:
    mov si, msg_e820
print_error:
    lodsb
    test al, al
    jz .halt
    mov ah, 0x0E
    int 0x10
    jmp print_error
.halt:
    cli
    hlt
    jmp .halt

boot_drive db 0
boot_image_lba dd 0
kernel_sectors dd 0
kernel_bytes dd 0
remaining dd 0
current_lba dd 0
stage_lo dd 0

msg_header db 'ZevBoot BIOS: invalid header',0
msg_disk db 'ZevBoot BIOS: disk read failed',0
msg_large db 'ZevBoot BIOS: kernel too large',0
msg_e820 db 'ZevBoot BIOS: E820 unavailable',0

align 4
dap:
    db 0x10,0
.count: dw 0
.offset: dw 0
.segment: dw 0
.buffer_lo: dd 0
.buffer_hi: dd 0
.lba_lo: dd 0
.lba_hi: dd 0

align 8
gdt:
    dq 0
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt_ptr:
    dw gdt_ptr-gdt-1
    dd gdt

times STAGE2_SECTORS*512-($-$$) db 0

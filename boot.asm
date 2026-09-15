; ZevOS - Multiboot2 x86_64 entry point
; GRUB enters us in 32-bit protected mode. This trampoline enables
; PAE + long mode, installs identity-mapped 2 MiB pages, then calls C.

BITS 32

section .multiboot
align 8
mb2_header:
    dd 0xE85250D6              ; Multiboot2 magic
    dd 0                       ; architecture = i386
    dd mb2_header_end - mb2_header
    dd -(0xE85250D6 + 0 + (mb2_header_end - mb2_header))
    dw 0                       ; end tag type
    dw 0
    dd 8
mb2_header_end:

section .text
align 16
global _start
extern kernel_main

_start:
    cli
    mov esp, stack_top

    ; GRUB supplies EAX=Multiboot2 magic, EBX=information structure.
    mov [multiboot_magic], eax
    mov [multiboot_info], ebx

    ; Build identity mapping for the first 1 GiB with 2 MiB pages.
    mov eax, page_table_l4
    mov cr3, eax

    mov eax, page_table_l3
    or eax, 0x3
    mov [page_table_l4], eax

    mov eax, page_table_l2
    or eax, 0x3
    mov [page_table_l3], eax

    xor ecx, ecx
.fill_pd:
    mov eax, ecx
    shl eax, 21               ; physical address = index * 2 MiB
    or eax, 0x83              ; present | writable | huge (2 MiB)
    mov [page_table_l2 + ecx * 8], eax
    mov dword [page_table_l2 + ecx * 8 + 4], 0
    inc ecx
    cmp ecx, 512              ; 512 * 2 MiB = 1 GiB
    jne .fill_pd

    ; Enable PAE.
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Enable EFER.LME.
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Enable paging.
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    lgdt [gdt64.pointer]
    jmp gdt64.code:long_mode_start

BITS 64
long_mode_start:
    mov ax, gdt64.data
    mov ds, ax
    mov es, ax
    mov ss, ax

    mov rsp, stack_top
    and rsp, -16

    ; SysV x86_64 ABI: first two C arguments are RDI and RSI.
    mov edi, dword [multiboot_magic]
    mov esi, dword [multiboot_info]
    call kernel_main

.hang:
    cli
    hlt
    jmp .hang

section .rodata
align 8
gdt64:
    dq 0
.code equ $ - gdt64
    dq 0x00AF9A000000FFFF
gdt64.data equ $ - gdt64
    dq 0x00CF92000000FFFF
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

section .bss
align 4096
page_table_l4: resq 512
page_table_l3: resq 512
page_table_l2: resq 512

align 16
stack_bottom: resb 16384
stack_top:

align 4
multiboot_magic: resd 1
multiboot_info:  resd 1

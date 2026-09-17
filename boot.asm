; ZevOS - Multiboot2 x86_64 entry point
; GRUB enters us in 32-bit protected mode. This trampoline enables
; PAE + long mode, installs identity-mapped 2 MiB pages, then calls C.
; Direct HDA boot enters the same 32-bit entry with the Multiboot magic
; supplied by hda_boot.asm and a null info pointer.

BITS 32

section .multiboot
align 8
mb2_header:
    dd 0xE85250D6
    dd 0
    dd mb2_header_end - mb2_header
    dd -(0xE85250D6 + 0 + (mb2_header_end - mb2_header))
    dw 0
    dw 0
    dd 8
mb2_header_end:

section .text
align 16
global _start
extern kernel_main
extern tss_init
extern kernel_bss_start
extern kernel_bss_end

_start:
    cli
    mov esp, stack_top
    mov [multiboot_magic], eax
    mov [multiboot_info], ebx

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
    shl eax, 21
    or eax, 0x83
    mov [page_table_l2 + ecx * 8], eax
    mov dword [page_table_l2 + ecx * 8 + 4], 0
    inc ecx
    cmp ecx, 512
    jne .fill_pd

    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

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

    ; GRUB normally gives us zeroed BSS. Direct HDA boot does not, so
    ; explicitly clear it before any global/BSS-backed subsystem runs.
    cld
    mov rdi, kernel_bss_start
    mov rcx, kernel_bss_end
    sub rcx, rdi
    shr rcx, 3
    xor eax, eax
    rep stosq

    call tss_init

    mov edi, dword [multiboot_magic]
    mov esi, dword [multiboot_info]
    call kernel_main

.hang:
    cli
    hlt
    jmp .hang

section .rodata
align 8
global gdt64
gdt64:
    dq 0
.code equ $ - gdt64
    dq 0x00AF9A000000FFFF       ; kernel code, selector 0x08
.data equ $ - gdt64
    dq 0x00CF92000000FFFF       ; kernel data, selector 0x10
.user_code equ $ - gdt64
    dq 0x00AFFA000000FFFF       ; user code, selector 0x18 / RPL3=0x1B
.user_data equ $ - gdt64
    dq 0x00CFF2000000FFFF       ; user data, selector 0x20 / RPL3=0x23
.tss_low equ $ - gdt64
times 16 db 0
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

align 16
global tss_stack_top
tss_stack_bottom: resb 16384
tss_stack_top:

align 8
global tss64
tss64:
    resb 104

align 4
multiboot_magic: resd 1
multiboot_info:  resd 1

section .note.GNU-stack noalloc noexec nowrite progbits

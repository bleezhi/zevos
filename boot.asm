; ZevOS - Multiboot2 x86_64 entry point
; GRUB enters us in 32-bit protected mode. This trampoline enables
; PAE + long mode, installs identity-mapped 2 MiB pages, then calls C.

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

_start:
    cli
    mov esp, stack_top
    mov [multiboot_magic], eax
    mov [multiboot_info], ebx

    ; Identity-map the first 1 GiB with 2 MiB pages.
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

    ; Install the long-mode TSS. It provides RSP0 when ring 3
    ; takes an interrupt back into the kernel.
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
    dw 0                        ; limit low, filled by tss_init
    dw 0                        ; base low, filled by tss_init
    db 0                        ; base mid
    db 0x89                     ; present, available 64-bit TSS
    db 0                        ; limit high + flags
    db 0                        ; base high
    dd 0                        ; base upper 32 bits
    dd 0                        ; reserved
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
tss_stack_bottom: resb 16384
tss_stack_top:

align 8
tss64:
    resb 104

align 4
multiboot_magic: resd 1
multiboot_info:  resd 1

section .note.GNU-stack noalloc noexec nowrite progbits

; Native ZevBoot UEFI -> ZevOS kernel entry.
; UEFI has already entered 64-bit long mode. We install our own GDT,
; identity-map the first 4 GiB, load the TSS, then call kernel_main().
BITS 64

section .text
global uefi_start
extern gdt64
extern tss_init
extern kernel_main
extern page_table_l4
extern page_table_l3
extern page_table_l2

uefi_start:
    cli
    lgdt [rel uefi_gdt_pointer]

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax

    mov rsp, stack_top
    and rsp, -16

    ; Clear the kernel BSS so page tables/stacks/TSS are deterministic.
    cld
    extern kernel_bss_start
    extern kernel_bss_end
    mov rdi, kernel_bss_start
    mov rcx, kernel_bss_end
    sub rcx, rdi
    shr rcx, 3
    xor eax, eax
    rep stosq

    ; Build a 4 GiB identity map using 2 MiB pages.
    mov rdi, page_table_l4
    xor eax, eax
    mov ecx, 512
    rep stosq
    mov rdi, page_table_l3
    xor eax, eax
    mov ecx, 512
    rep stosq
    mov rdi, page_table_l2
    xor eax, eax
    mov ecx, 2048
    rep stosq

    mov rax, page_table_l3
    or rax, 0x3
    mov [page_table_l4], rax

    mov rax, page_table_l2
    or rax, 0x3
    mov [page_table_l3], rax
    mov rax, page_table_l2 + 4096
    or rax, 0x3
    mov [page_table_l3 + 8], rax
    mov rax, page_table_l2 + 8192
    or rax, 0x3
    mov [page_table_l3 + 16], rax
    mov rax, page_table_l2 + 12288
    or rax, 0x3
    mov [page_table_l3 + 24], rax

    xor rcx, rcx
.map_pd:
    mov rax, rcx
    shl rax, 21
    or rax, 0x83
    mov [page_table_l2 + rcx*8], rax
    inc rcx
    cmp rcx, 2048
    jne .map_pd

    mov rax, page_table_l4
    mov cr3, rax

    call tss_init

    mov rdi, r12
    call kernel_main

.hang:
    cli
    hlt
    jmp .hang

section .data
align 8
uefi_gdt_pointer:
    dw 0x37
    dq gdt64

section .note.GNU-stack noalloc noexec nowrite progbits

section .bss
align 16
stack_bottom: resb 16384
stack_top:

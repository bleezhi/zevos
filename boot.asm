BITS 32

; ZevOS native BIOS kernel trampoline.
; ZevBoot BIOS stage 2 enters here in 32-bit protected mode with:
;   EBX = struct zev_boot_info *
; The trampoline creates a 64-bit environment and calls kernel_main(info).

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
    mov [boot_info_ptr], ebx

    mov eax, page_table_l4
    mov cr3, eax

    mov eax, page_table_l3
    or eax, 0x3
    mov [page_table_l4], eax

    xor ecx, ecx
    mov eax, page_table_l2
.fill_l3:
    mov edx, eax
    or edx, 0x3
    mov [page_table_l3 + ecx * 8], edx
    add eax, 4096
    inc ecx
    cmp ecx, 4
    jne .fill_l3

    xor ecx, ecx
.fill_pd:
    mov eax, ecx
    shl eax, 21
    or eax, 0x83
    mov [page_table_l2 + ecx * 8], eax
    mov dword [page_table_l2 + ecx * 8 + 4], 0
    inc ecx
    cmp ecx, 2048
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

    cld
    mov rdi, kernel_bss_start
    mov rcx, kernel_bss_end
    sub rcx, rdi
    shr rcx, 3
    xor eax, eax
    rep stosq

    call tss_init

    mov rdi, [rel boot_info_ptr]
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
    dq 0x00AF9A000000FFFF
.data equ $ - gdt64
    dq 0x00CF92000000FFFF
.user_code equ $ - gdt64
    dq 0x00AFFA000000FFFF
.user_data equ $ - gdt64
    dq 0x00CFF2000000FFFF
.tss_low equ $ - gdt64
    times 16 db 0
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

section .bss
align 4096
global page_table_l4
page_table_l4: resq 512
global page_table_l3
page_table_l3: resq 512
global page_table_l2
page_table_l2: resq 512 * 4

align 16
stack_bottom: resb 16384
global stack_top
stack_top:

align 16
global tss_stack_top
tss_stack_bottom: resb 16384
tss_stack_top:

align 8
global tss64
tss64:
    resb 104

align 8
boot_info_ptr: resq 1

section .note.GNU-stack noalloc noexec nowrite progbits

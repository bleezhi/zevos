BITS 64

section .text
global tss_init
extern gdt64
extern tss64
extern tss_stack_top

tss_init:
    ; Zero the TSS.
    lea rdi, [rel tss64]
    xor eax, eax
    mov ecx, 104 / 8
    rep stosq

    ; TSS.rsp0 is the kernel stack used on a ring 3 -> ring 0 transition.
    mov rax, tss_stack_top
    mov [rel tss64 + 4], rax

    ; I/O bitmap offset beyond the TSS means no bitmap is present.
    mov word [rel tss64 + 102], 104

    ; Build the 16-byte 64-bit available TSS descriptor at GDT offset 0x28.
    mov rax, tss64
    mov rdx, rax
    mov rcx, rax
    and eax, 0xFFFF
    and edx, 0xFFFFFF000000
    shr rdx, 16
    and ecx, 0xFFFFFFFF00000000
    shr rcx, 32

    mov r8, 103
    or rax, r8
    ; Rebuild low descriptor from limit/base fields.
    xor rax, rax
    mov ax, 103
    mov r8, tss64
    mov r9, r8
    and r9d, 0xFFFF
    shl r9, 16
    or rax, r9
    mov r9, r8
    shr r9, 24
    and r9d, 0xFF
    shl r9, 56
    or rax, r9
    mov r9, 0x89
    shl r9, 40
    or rax, r9
    mov r9, tss64
    shr r9, 16
    and r9d, 0xFF
    shl r9, 56
    or rax, r9
    ; Limit high is zero for a 104-byte TSS.
    mov [rel gdt64 + 0x28], rax

    mov rax, tss64
    shr rax, 32
    mov [rel gdt64 + 0x30], rax

    lgdt [rel gdt_pointer]
    mov ax, 0x28
    ltr ax
    ret

section .data
align 8
gdt_pointer:
    dw 0x37
    dq gdt64

section .note.GNU-stack noalloc noexec nowrite progbits

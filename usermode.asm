; ZevOS ring 3 transition helpers
BITS 64

section .text
global user_enter
global user_launch

; void user_enter(entry, stack, user_cs, user_ss)
; RDI=entry, RSI=stack, RDX=user_cs, RCX=user_ss
user_enter:
    mov ax, cx
    mov ds, ax
    mov es, ax

    push rcx
    push rsi
    pushfq
    pop rax
    or rax, 0x200
    push rax
    push rdx
    push rdi
    iretq

; void user_launch(cr3, entry, stack)
; RDI=CR3, RSI=user RIP, RDX=user RSP.
; Builds the complete initial privilege-transition frame and never returns.
user_launch:
    mov cr3, rdi

    mov ax, 0x23
    mov ds, ax
    mov es, ax

    ; Initial ring-3 interrupt-return frame:
    ; SS, RSP, RFLAGS, CS, RIP.
    push qword 0x23
    push rdx
    pushfq
    pop rax
    or rax, 0x200
    push rax
    push qword 0x1B
    push rsi
    iretq

section .note.GNU-stack noalloc noexec nowrite progbits

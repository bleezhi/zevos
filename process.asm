; ZevOS kernel process context helpers
BITS 64

section .text
global process_switch
global process_exit_handoff

extern vmm_kernel_cr3
extern process_exit_idle
extern tss_stack_top

; void process_switch(uint64_t *old_rsp, uint64_t new_rsp)
; Save the SysV callee-saved registers and switch to another kernel stack.
process_switch:
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov [rdi], rsp
    mov rsp, rsi

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    ret

; void process_exit_handoff(uint64_t status)
; Abandon the terminated task's ring-0 stack, restore the kernel address
; space, and continue on the dedicated bootstrap/TSS stack.
process_exit_handoff:
    push rdi
    call vmm_kernel_cr3
    mov cr3, rax
    pop rdi

    mov rsp, tss_stack_top
    and rsp, -16
    call process_exit_idle

.hang:
    cli
    hlt
    jmp .hang

section .note.GNU-stack noalloc noexec nowrite progbits

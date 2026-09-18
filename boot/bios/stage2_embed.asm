BITS 64
section .rodata
align 16
global zevboot_stage2_start
global zevboot_stage2_end
zevboot_stage2_start:
    incbin "boot/bios/stage2.bin"
zevboot_stage2_end:
section .note.GNU-stack noalloc noexec nowrite progbits

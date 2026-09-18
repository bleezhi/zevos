BITS 64
section .rodata
align 16
global zevboot_stage1_start
global zevboot_stage1_end
zevboot_stage1_start:
    incbin "boot/bios/boot.bin"
zevboot_stage1_end:
section .note.GNU-stack noalloc noexec nowrite progbits

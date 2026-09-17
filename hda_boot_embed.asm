BITS 64
section .rodata
align 16
global hda_boot_start
global hda_boot_end
hda_boot_start:
    incbin "hda_boot.bin"
hda_boot_end:
section .note.GNU-stack noalloc noexec nowrite progbits

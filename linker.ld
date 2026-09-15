ENTRY(_start)

SECTIONS
{
    . = 1M;

    /* GRUB Multiboot2 requires the header to be early in the image. */
    .multiboot ALIGN(8) :
    {
        KEEP(*(.multiboot))
    }

    .text ALIGN(16) :
    {
        *(.text*)
    }

    .rodata ALIGN(16) :
    {
        *(.rodata*)
    }

    .data ALIGN(16) :
    {
        *(.data*)
    }

    .bss ALIGN(16) :
    {
        *(COMMON)
        *(.bss*)
    }
}

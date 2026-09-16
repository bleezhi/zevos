ENTRY(_start)

SECTIONS
{
    . = 1M;
    kernel_start = .;

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

    /* Embedded bytes copied into the first userspace code page. */
    .userprog ALIGN(16) :
    {
        KEEP(*(.userprog))
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

    . = ALIGN(4096);
    kernel_end = .;
}

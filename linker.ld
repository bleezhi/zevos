ENTRY(_start)

SECTIONS
{
    . = 1M;
    kernel_start = .;

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

    .userprog ALIGN(16) :
    {
        KEEP(*(.userprog))
    }

    .data ALIGN(16) :
    {
        *(.data*)
    }

    . = ALIGN(4096);
    kernel_bss_start = .;
    .bss ALIGN(16) :
    {
        *(COMMON)
        *(.bss*)
    }
    kernel_bss_end = .;

    . = ALIGN(4096);
    kernel_end = .;
}

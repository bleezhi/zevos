# ZevBoot UEFI

`ZevBoot.efi` is the native UEFI loader for ZevOS.

Expected EFI System Partition layout:

    EFI/BOOT/BOOTX64.EFI
    EFI/ZEVOS/KERNEL.ELF

The loader:

1. opens the kernel ELF from the same filesystem as the EFI application,
2. validates and loads its PT_LOAD segments at the linked address,
3. obtains GOP framebuffer information,
4. obtains the final UEFI memory map,
5. converts it to ZevOS memory-map entries,
6. calls ExitBootServices(),
7. enters the kernel's `uefi_start` entry point.

The kernel then installs its own page tables and GDT/TSS before starting
the normal ZevOS initialization sequence.

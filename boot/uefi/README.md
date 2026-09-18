# ZevBoot UEFI

This directory is the UEFI loader foundation. It is written for EDK2 and is intentionally kept separate from the kernel build until the ZevBoot protocol is finalized.

Expected layout on the EFI System Partition:

    EFI/ZEVOS/ZEVBOOT.EFI
    EFI/ZEVOS/KERNEL.ELF

The loader will provide GOP framebuffer information, the UEFI memory map, the kernel image, and boot-device information before calling ExitBootServices().

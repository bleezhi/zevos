# ZevBoot

ZevOS no longer depends on GRUB on the nightly branch.

The native boot tree contains two firmware paths:

- `bios/` — legacy BIOS ZevBoot stage 1 + stage 2
- `uefi/` — EDK2 UEFI application

Both loaders hand the kernel a shared `struct zev_boot_info` from `zevboot.h`.

BIOS layout inside the native boot image:

    LBA 0      ZevBoot stage 1
    LBA 1      ZBOT header
    LBA 2-17   ZevBoot stage 2
    LBA 18+    kernel image

The BIOS loader uses E820 to populate the ZevBoot memory map. The UEFI
loader uses GOP and the UEFI memory map, then calls ExitBootServices().

The kernel has its own 64-bit entry, page tables, TSS setup, and framebuffer
terminal, so neither path needs Multiboot2 or GRUB state.

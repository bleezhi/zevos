# ZevOS

A tiny x86_64 hobby operating system built from scratch with C and NASM assembly.

## Current status

- GRUB Multiboot2 boot
- x86_64 long-mode transition
- Temporary identity-mapped 2 MiB pages
- 16 KiB kernel stack
- Freestanding C kernel
- Direct VGA text-mode output
- Reproducible ISO build with Make
- GitHub Actions ISO build artifact

## Build locally

On Debian/Ubuntu, install:

```sh
sudo apt install gcc binutils nasm grub-common grub-pc-bin xorriso make
```

Then run:

```sh
make
```

The resulting bootable image is `os.iso`.

To verify the Multiboot2 header without creating the ISO:

```sh
make check
```

## Run with QEMU

If QEMU is installed:

```sh
qemu-system-x86_64 -cdrom os.iso
```

The kernel should display:

```text
ZevOS booted successfully!
```

## Project layout

```text
boot.asm                 Multiboot2 entry + long-mode setup
kernel.c                 Freestanding C kernel
linker.ld                Kernel memory layout
grub.cfg                 GRUB boot menu
Makefile                 Local build system
.github/workflows/       Automated ISO builds
```

ZevOS is a hobby/learning project and is currently intended for BIOS/GRUB-style booting.

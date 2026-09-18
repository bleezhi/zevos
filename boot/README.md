# ZevBoot

ZevOS is moving away from a GRUB dependency. The boot tree contains the native ZevOS boot protocol and firmware-specific loaders.

- bios/: native legacy-BIOS boot path
- uefi/: UEFI application path

The existing HDA stage-0 loader is the first native BIOS implementation. The long-term protocol passes a ZevBootInfo structure instead of Multiboot2 state.

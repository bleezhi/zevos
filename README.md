# ZevOS

**ZevOS is an x86_64 Linux distribution project.** The Linux-distro effort is being developed on the `linux-distro` branch. The original freestanding C/NASM hobby kernel remains preserved on `main`, `nightly`, and the historical branches.

## Direction

ZevOS starts as an Arch-compatible binary-package distribution with its own release identity, configuration, installer/live image, and curated package repository. It will use upstream Linux and upstream packages wherever practical instead of maintaining a private fork of the entire Linux ecosystem.

The first milestone is a reproducible, bootable x86_64 base image. A ZevOS-owned package repository and Zev-specific tooling follow once the base image is reliable.

## Initial platform decisions

- Architecture: x86_64
- Kernel: upstream Linux, initially from the Arch-compatible package ecosystem
- Package format/manager: pacman-compatible binary packages
- Base packages: upstream Arch-compatible packages
- Init: systemd initially; it gives us a complete, supported userspace
- Boot: UEFI first; BIOS support follows
- Filesystem: ext4 by default
- Build target: bootable live/install ISO, tested with QEMU/OVMF

These are initial engineering choices, not permanent constraints.

## Repository layout

- `distro/`: architecture and release policy
- `packages/`: ZevOS-maintained package recipes
- `config/`: image and system configuration
- `scripts/`: build and validation helpers
- `legacy-kernel/`: reserved for the original freestanding kernel if/when it is moved

The existing hobby-kernel sources are intentionally preserved.

## Build the first ZevOS package

On an Arch Linux build host with `base-devel` installed:

```sh
cd packages/zevos-release
makepkg --cleanbuild
```

Install it only in a disposable test VM:

```sh
sudo pacman -U ./zevos-release-*.pkg.tar.zst
```

## Current status

This branch begins the Linux distribution migration. It is **not yet a bootable ISO** and must not be installed on real hardware.

Milestones:
1. Build and test the ZevOS release-identity package.
2. Add a pinned, repeatable base-rootfs build.
3. Produce a UEFI live ISO with upstream Linux and systemd.
4. Add an installer with explicit disk selection and destructive-action confirmation.
5. Publish signed ZevOS packages and repository metadata.
6. Develop ZevOS-specific tools and desktop components.

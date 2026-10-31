# ZevOS distribution architecture

## Release model

ZevOS begins as a rolling x86_64 binary distribution. The first implementation reuses the Arch package format and pacman client while maintaining a small ZevOS-owned package set. Upstream repositories remain the source of general-purpose system packages.

Do not mirror or rebuild the entire Arch package set initially. A ZevOS repository should contain only packages that ZevOS maintains or intentionally overrides.

## Base system

The initial base is expected to include Linux, firmware, systemd, systemd-boot, pacman, glibc, bash, coreutils, util-linux, e2fsprogs, iproute2, NetworkManager, sudo, nano and less. SSH should be disabled by default.

Pin package versions and repository snapshots for image builds. Do not claim reproducibility until the build records exact package database snapshots and input checksums.

## ZevOS-owned components

- `zevos-release`: OS identity and release metadata
- `zpkg`: possible higher-level frontend; pacman remains the transaction engine
- `zinit`: future service manager experiment; systemd remains PID 1 until zinit has supervision, dependency ordering, logging, shutdown and recovery
- `zde`, `dsplayed`, `windows`, `zcomp`: future desktop stack, not prerequisites for first boot

## Safety rules

- Never run an installer against the development host's disks.
- Build and test images with disposable virtual disks.
- The installer must display the exact target device and require a second confirmation before formatting.
- Keep the freestanding ZevOS kernel history intact.
- Do not replace upstream package signatures with unsigned downloads.

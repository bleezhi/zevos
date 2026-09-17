# ZevOS userspace binaries

This directory contains the source definitions for programs installed into `/bin` by the ZevOS VFS image builder.

Stage 0 binaries are intentionally tiny freestanding ELF64 programs. The long-term goal is for these files to be built as real ELF files and copied into `/bin` on the installed filesystem.

Current layout:

- `echo.asm` — userspace echo test
- `true.asm` — exits successfully
- `false.asm` — exits with failure status
- `pwd.asm` — userspace pwd test
- `ls.asm` — userspace ls test
- `cat.asm` — userspace cat test
- `zinit.asm` — userspace init bootstrap
- `dsplayed.asm` — Stage 0 display server bootstrap
- `sh.asm` — future userspace shell

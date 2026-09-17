CC      := gcc
LD      := ld
AS      := nasm
OBJCOPY := objcopy
GRUB    := grub-file
RESCUE  := grub-mkrescue

CFLAGS  := -ffreestanding -fno-stack-protector -fno-pie -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mcmodel=small -O2 -Wall -Wextra
LDFLAGS := -T linker.ld -nostdlib -z max-page-size=0x1000

OBJS := boot.o interrupts.o interrupts_c.o kernel.o terminal.o terminal_backspace.o keyboard.o shell.o installer.o vfs.o user_bins.o pmm.o heap.o process.o process_asm.o usermode.o usermode_asm.o user_program.o tss.o vmm.o elf.o fd.o syscall.o syscall_asm.o zinit.o ata.o zevfs.o hda_boot_embed.o

.PHONY: all clean check iso

all: os.iso

boot.o: boot.asm
	$(AS) -f elf64 $< -o $@

hda_boot.bin: hda_boot.asm
	$(AS) -f bin $< -o $@
	@test "$$(wc -c < $@)" -eq 512

hda_boot_embed.o: hda_boot_embed.asm hda_boot.bin
	$(AS) -f elf64 $< -o $@

interrupts.o: interrupts.asm
	$(AS) -f elf64 $< -o $@

process_asm.o: process.asm
	$(AS) -f elf64 $< -o $@

usermode_asm.o: usermode.asm
	$(AS) -f elf64 $< -o $@

user_program.o: user_program.asm
	$(AS) -f elf64 $< -o $@

tss.o: tss.asm
	$(AS) -f elf64 $< -o $@

syscall_asm.o: syscall.asm
	$(AS) -f elf64 $< -o $@

interrupts_c.o: interrupts.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel.elf: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

check: kernel.elf
	$(GRUB) --is-x86-multiboot2 kernel.elf
	@echo "Multiboot2 header OK"

iso: os.iso

os.iso: kernel.elf grub.cfg
	$(GRUB) --is-x86-multiboot2 kernel.elf
	rm -rf iso
	mkdir -p iso/boot/grub
	cp kernel.elf iso/boot/kernel.elf
	cp grub.cfg iso/boot/grub/grub.cfg
	$(RESCUE) -o $@ iso

clean:
	rm -rf *.o *.elf *.iso *.bin iso

CC      := gcc
LD      := ld
AS      := nasm
GRUB    := grub-file
RESCUE  := grub-mkrescue

CFLAGS  := -ffreestanding -fno-stack-protector -fno-pie -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mcmodel=kernel -O2 -Wall -Wextra
LDFLAGS := -T linker.ld -nostdlib -z max-page-size=0x1000

.PHONY: all clean check iso

all: os.iso

boot.o: boot.asm
	$(AS) -f elf64 $< -o $@

kernel.o: kernel.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel.elf: boot.o kernel.o linker.ld
	$(LD) $(LDFLAGS) -o $@ boot.o kernel.o

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
	rm -rf *.o *.elf *.iso iso

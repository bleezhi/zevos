CC      := gcc
LD      := ld
AS      := nasm
OBJCOPY := objcopy
PYTHON  := python3
XORRISO := xorriso

CFLAGS  := -ffreestanding -fno-stack-protector -fno-pie -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mcmodel=small -O2 -Wall -Wextra
LDFLAGS := -T linker.ld -nostdlib -z max-page-size=0x1000

HDA_IMAGE := zevos-hda.img
HDA_SIZE_MB := 64
EDK2_DIR ?= edk2
UEFI_APP := boot/uefi/ZevBoot.efi
BIOS_STAGE2_SECTORS := 16
BIOS_KERNEL_LBA := 18

OBJS := boot.o uefi_entry.o zevboot_stage1_embed.o zevboot_stage2_embed.o interrupts.o interrupts_c.o kernel.o terminal.o terminal_backspace.o keyboard.o shell.o installer.o mainmenu.o vfs.o user_bins.o pmm.o heap.o process.o process_asm.o usermode.o usermode_asm.o user_program.o tss.o vmm.o elf.o fd.o syscall.o syscall_asm.o zinit.o ata.o zevfs.o

.PHONY: all clean check iso hda uefi bios

all: os.iso

boot.o: boot.asm
	$(AS) -f elf64 $< -o $@

uefi_entry.o: uefi_entry.asm
	$(AS) -f elf64 $< -o $@

zevboot_stage1_embed.o: boot/bios/stage1_embed.asm boot/bios/boot.bin
	$(AS) -f elf64 $< -o $@

zevboot_stage2_embed.o: boot/bios/stage2_embed.asm boot/bios/stage2.bin
	$(AS) -f elf64 $< -o $@

boot/bios/boot.bin: boot/bios/boot.asm
	$(AS) -f bin $< -o $@
	@test "$$(wc -c < $@)" -eq 512

boot/bios/stage2.bin: boot/bios/stage2.asm
	$(AS) -f bin $< -o $@
	@test "$$(wc -c < $@)" -eq $$(($(BIOS_STAGE2_SECTORS) * 512))

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

kernel.bin: kernel.elf
	$(OBJCOPY) -O binary $< $@

check: kernel.elf
	@echo "ZevOS kernel ELF OK"
	@file kernel.elf

boot/uefi/ZevBoot.efi: boot/uefi/ZevBoot.dsc boot/uefi/ZevBoot.inf boot/uefi/ZevBoot.c zevboot.h
	@test -d "$(EDK2_DIR)/BaseTools"
	@$(MAKE) -C "$(EDK2_DIR)/BaseTools"
	@cd "$(CURDIR)" && export WORKSPACE="$(CURDIR)" && export PACKAGES_PATH="$(CURDIR)/$(EDK2_DIR)" && export EDK_TOOLS_PATH="$(CURDIR)/$(EDK2_DIR)/BaseTools" && . "$(CURDIR)/$(EDK2_DIR)/edksetup.sh" && build -p boot/uefi/ZevBoot.dsc -a X64 -t GCC5 -b RELEASE
	@cp Build/ZevBoot/RELEASE_GCC5/X64/ZevBoot.efi $@

uefi: $(UEFI_APP)

zevboot-bios.bin: kernel.bin boot/bios/boot.bin boot/bios/stage2.bin
	@$(PYTHON) -c "import struct; k=open('kernel.bin','rb').read(); n=(len(k)+511)//512; h=bytearray(512); h[0:4]=b'ZBOT'; h[4:8]=struct.pack('<I',n); h[8:12]=struct.pack('<I',len(k)); h[12:16]=struct.pack('<I',$(BIOS_KERNEL_LBA)); open('zevboot-header.bin','wb').write(h); open('zevboot-kernel.bin','wb').write(k+b'\0'*(n*512-len(k)))"
	@cat boot/bios/boot.bin zevboot-header.bin boot/bios/stage2.bin zevboot-kernel.bin > $@
	@rm -f zevboot-header.bin zevboot-kernel.bin

bios: zevboot-bios.bin

efiboot.img: $(UEFI_APP) kernel.elf
	@rm -f $@
	@dd if=/dev/zero of=$@ bs=1M count=16 status=none
	@mkfs.fat -F 16 $@ >/dev/null
	@mkdir -p .efi-image/EFI/BOOT .efi-image/EFI/ZEVOS
	@cp $(UEFI_APP) .efi-image/EFI/BOOT/BOOTX64.EFI
	@cp kernel.elf .efi-image/EFI/ZEVOS/KERNEL.ELF
	@mcopy -s -i $@ .efi-image/* ::/
	@rm -rf .efi-image

iso: os.iso

os.iso: kernel.elf zevboot-bios.bin efiboot.img
	@rm -rf iso
	@mkdir -p iso
	@$(XORRISO) -as mkisofs -R -J -V ZEVOS \
		-b zevboot-bios.bin -no-emul-boot \
		-boot-info-table -eltorito-alt-boot -e efiboot.img -no-emul-boot \
		-isohybrid-gpt-basdat -o $@ iso

hda: zevboot-bios.bin
	@dd if=/dev/zero of=$(HDA_IMAGE) bs=1M count=$(HDA_SIZE_MB) status=none
	@dd if=zevboot-bios.bin of=$(HDA_IMAGE) bs=512 conv=notrunc status=none
	@echo "Created $(HDA_IMAGE) using native ZevBoot BIOS"

clean:
	rm -rf *.o *.elf *.iso *.bin *.img iso Build .efi-image zevboot-header.bin zevboot-kernel.bin

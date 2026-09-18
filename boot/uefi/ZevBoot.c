/* Native ZevBoot UEFI loader.
 * Loads the ZevOS ELF64 kernel at its linked physical address, builds the
 * ZevBootInfo handoff, exits boot services, and enters the kernel.
 */
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/GraphicsOutput.h>
#include <Guid/FileInfo.h>
#include "../../zevboot.h"

#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define ET_EXEC 2
#define EM_X86_64 0x3E
#define PT_LOAD 1
#define EFI_FILE_MODE_READ 0x0000000000000001ULL

typedef struct {
    UINT8 ident[16];
    UINT16 type, machine;
    UINT32 version;
    UINT64 entry, phoff, shoff;
    UINT32 flags;
    UINT16 ehsize, phentsize, phnum, shentsize, shnum, shstrndx;
} ELF64_EHDR;

typedef struct {
    UINT32 type, flags;
    UINT64 offset, vaddr, paddr, filesz, memsz, align;
} ELF64_PHDR;

static EFI_STATUS read_file(EFI_FILE_PROTOCOL *Root, CHAR16 *Path, VOID **Data, UINTN *Size)
{
    EFI_STATUS Status;
    EFI_FILE_PROTOCOL *File = NULL;
    EFI_FILE_INFO *Info = NULL;
    UINTN InfoSize = 0;
    VOID *Buffer;

    Status = Root->Open(Root, &File, Path, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(Status)) return Status;

    Status = File->GetInfo(File, &gEfiFileInfoGuid, &InfoSize, NULL);
    if (Status != EFI_BUFFER_TOO_SMALL) {
        File->Close(File);
        return Status;
    }

    Info = AllocatePool(InfoSize);
    if (!Info) {
        File->Close(File);
        return EFI_OUT_OF_RESOURCES;
    }

    Status = File->GetInfo(File, &gEfiFileInfoGuid, &InfoSize, Info);
    if (EFI_ERROR(Status)) {
        FreePool(Info);
        File->Close(File);
        return Status;
    }

    *Size = (UINTN)Info->FileSize;
    Buffer = AllocatePool(*Size);
    if (!Buffer) {
        FreePool(Info);
        File->Close(File);
        return EFI_OUT_OF_RESOURCES;
    }

    UINTN ReadSize = *Size;
    Status = File->Read(File, &ReadSize, Buffer);
    FreePool(Info);
    File->Close(File);

    if (EFI_ERROR(Status) || ReadSize != *Size) {
        FreePool(Buffer);
        return EFI_LOAD_ERROR;
    }

    *Data = Buffer;
    return EFI_SUCCESS;
}

static EFI_STATUS load_elf(VOID *Data, UINTN Size, UINT64 *Entry,
                           EFI_PHYSICAL_ADDRESS *LoadBase,
                           EFI_PHYSICAL_ADDRESS *LoadEnd)
{
    ELF64_EHDR *E = (ELF64_EHDR *)Data;
    UINT64 Min = ~0ULL, Max = 0;

    if (Size < sizeof(*E) ||
        E->ident[0] != 0x7F || E->ident[1] != 'E' ||
        E->ident[2] != 'L' || E->ident[3] != 'F' ||
        E->ident[4] != ELFCLASS64 || E->ident[5] != ELFDATA2LSB ||
        E->type != ET_EXEC || E->machine != EM_X86_64 ||
        E->phentsize != sizeof(ELF64_PHDR) || E->phnum == 0)
        return EFI_LOAD_ERROR;

    if (E->phoff > Size ||
        (UINT64)E->phnum * E->phentsize > Size - E->phoff)
        return EFI_LOAD_ERROR;

    for (UINTN i = 0; i < E->phnum; ++i) {
        ELF64_PHDR *P = (ELF64_PHDR *)((UINT8 *)Data +
                                        E->phoff + i * E->phentsize);
        if (P->type != PT_LOAD) continue;
        if (P->memsz < P->filesz ||
            P->offset > Size ||
            P->filesz > Size - P->offset ||
            P->vaddr < 0x100000ULL)
            return EFI_LOAD_ERROR;
        if (P->vaddr + P->memsz < P->vaddr)
            return EFI_LOAD_ERROR;
        if (P->vaddr < Min) Min = P->vaddr;
        if (P->vaddr + P->memsz > Max) Max = P->vaddr + P->memsz;
    }

    if (Min == ~0ULL || Max <= Min || E->entry < Min || E->entry >= Max)
        return EFI_LOAD_ERROR;

    Min &= ~0xFFFULL;
    Max = (Max + 0xFFFULL) & ~0xFFFULL;

    EFI_PHYSICAL_ADDRESS Base = (EFI_PHYSICAL_ADDRESS)Min;
    UINTN Pages = (UINTN)((Max - Min) / 0x1000ULL);

    EFI_STATUS Status = gBS->AllocatePages(AllocateAddress, EfiLoaderData,
                                           Pages, &Base);
    if (EFI_ERROR(Status)) {
        Print(L"ZevBoot: kernel cannot be placed at %lx\r\n", Min);
        return Status;
    }

    SetMem((VOID *)(UINTN)Min, (UINTN)(Max - Min), 0);

    for (UINTN i = 0; i < E->phnum; ++i) {
        ELF64_PHDR *P = (ELF64_PHDR *)((UINT8 *)Data +
                                        E->phoff + i * E->phentsize);
        if (P->type != PT_LOAD) continue;
        CopyMem((VOID *)(UINTN)P->vaddr,
                (UINT8 *)Data + P->offset,
                (UINTN)P->filesz);
    }

    *Entry = E->entry;
    *LoadBase = (EFI_PHYSICAL_ADDRESS)Min;
    *LoadEnd = (EFI_PHYSICAL_ADDRESS)Max;
    return EFI_SUCCESS;
}

static UINTN convert_memory_map(EFI_MEMORY_DESCRIPTOR *Map, UINTN MapSize,
                                UINTN DescSize,
                                struct zev_memory_map_entry *Out,
                                UINTN Capacity)
{
    UINTN Count = 0;

    for (UINTN Offset = 0;
         Offset + DescSize <= MapSize;
         Offset += DescSize) {
        EFI_MEMORY_DESCRIPTOR *D =
            (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)Map + Offset);

        if (Count >= Capacity) break;

        Out[Count].base = D->PhysicalStart;
        Out[Count].length = D->NumberOfPages * 0x1000ULL;
        Out[Count].type =
            (D->Type == EfiConventionalMemory ||
             D->Type == EfiBootServicesCode ||
             D->Type == EfiBootServicesData ||
             D->Type == EfiLoaderCode ||
             D->Type == EfiLoaderData) ? 1 : 2;
        Out[Count].reserved = 0;
        ++Count;
    }

    return Count;
}

EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle,
                           IN EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS Status;
    EFI_LOADED_IMAGE_PROTOCOL *Loaded;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Fs;
    EFI_FILE_PROTOCOL *Root;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop = NULL;
    VOID *KernelFile = NULL;
    UINTN KernelSize = 0;
    UINT64 Entry;
    EFI_PHYSICAL_ADDRESS KernelBase, KernelEnd;

    Status = gBS->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid,
                                 (VOID **)&Loaded);
    if (EFI_ERROR(Status)) return Status;

    Status = gBS->HandleProtocol(Loaded->DeviceHandle,
                                 &gEfiSimpleFileSystemProtocolGuid,
                                 (VOID **)&Fs);
    if (EFI_ERROR(Status)) return Status;

    Status = Fs->OpenVolume(Fs, &Root);
    if (EFI_ERROR(Status)) return Status;

    Status = read_file(Root, L"\\EFI\\ZEVOS\\KERNEL.ELF",
                       &KernelFile, &KernelSize);
    if (EFI_ERROR(Status)) {
        Root->Close(Root);
        return Status;
    }

    Status = load_elf(KernelFile, KernelSize, &Entry,
                      &KernelBase, &KernelEnd);
    FreePool(KernelFile);
    if (EFI_ERROR(Status)) {
        Root->Close(Root);
        return Status;
    }

    Status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL,
                                 (VOID **)&Gop);
    if (EFI_ERROR(Status)) Gop = NULL;

    /* Get a generously sized buffer for the final memory map. */
    UINTN MapSize = 0, MapKey = 0, DescSize = 0;
    UINT32 DescVersion = 0;
    Status = gBS->GetMemoryMap(&MapSize, NULL, &MapKey,
                               &DescSize, &DescVersion);
    if (Status != EFI_BUFFER_TOO_SMALL) {
        Root->Close(Root);
        return Status;
    }

    MapSize += DescSize * 32;
    EFI_MEMORY_DESCRIPTOR *Map = AllocatePool(MapSize);
    if (!Map) {
        Root->Close(Root);
        return EFI_OUT_OF_RESOURCES;
    }

    UINTN MapBufferSize = MapSize;
    Status = gBS->GetMemoryMap(&MapBufferSize, Map, &MapKey,
                               &DescSize, &DescVersion);
    if (EFI_ERROR(Status)) {
        FreePool(Map);
        Root->Close(Root);
        return Status;
    }

    UINTN Capacity = MapBufferSize / DescSize;
    struct zev_memory_map_entry *ZevMap =
        AllocatePool(Capacity * sizeof(*ZevMap));
    if (!ZevMap) {
        FreePool(Map);
        Root->Close(Root);
        return EFI_OUT_OF_RESOURCES;
    }

    UINTN ZevCount = convert_memory_map(Map, MapBufferSize, DescSize,
                                         ZevMap, Capacity);

    struct zev_boot_info *Info = AllocatePool(sizeof(*Info));
    if (!Info) {
        FreePool(ZevMap);
        FreePool(Map);
        Root->Close(Root);
        return EFI_OUT_OF_RESOURCES;
    }

    ZeroMem(Info, sizeof(*Info));
    Info->magic = ZEV_BOOT_MAGIC;
    Info->version = ZEV_BOOT_VERSION;
    Info->memory_map = (UINT64)(UINTN)ZevMap;
    Info->memory_map_entries = ZevCount;
    Info->memory_map_entry_size = sizeof(*ZevMap);

    if (Gop && Gop->Mode && Gop->Mode->Info) {
        Info->framebuffer = Gop->Mode->FrameBufferBase;
        Info->framebuffer_width = Gop->Mode->Info->HorizontalResolution;
        Info->framebuffer_height = Gop->Mode->Info->VerticalResolution;
        Info->framebuffer_pitch =
            Gop->Mode->Info->PixelsPerScanLine * 4;
        Info->framebuffer_format =
            (UINT32)Gop->Mode->Info->PixelFormat;
    }

    Info->kernel_base = KernelBase;
    Info->kernel_end = KernelEnd;
    Info->boot_drive = 0;
    Info->flags = 2; /* UEFI */

    /*
     * Freeing the temporary first map changes the memory map, so obtain the
     * actual final map only after all allocations are finished. The map
     * buffer itself is retained until ExitBootServices.
     */
    FreePool(Map);
    Root->Close(Root);

    MapSize = 0;
    Status = gBS->GetMemoryMap(&MapSize, NULL, &MapKey,
                               &DescSize, &DescVersion);
    if (Status != EFI_BUFFER_TOO_SMALL) return Status;

    MapSize += DescSize * 32;
    Map = AllocatePool(MapSize);
    if (!Map) return EFI_OUT_OF_RESOURCES;

    MapBufferSize = MapSize;
    Status = gBS->GetMemoryMap(&MapBufferSize, Map, &MapKey,
                               &DescSize, &DescVersion);
    if (EFI_ERROR(Status)) return Status;

    ZevCount = convert_memory_map(Map, MapBufferSize, DescSize,
                                  ZevMap, Capacity);
    Info->memory_map_entries = ZevCount;

    Status = gBS->ExitBootServices(ImageHandle, MapKey);
    if (EFI_ERROR(Status)) return Status;

    /* Pass the handoff through R12 so uefi_start can preserve it while it
     * installs the kernel's own page tables. */
    VOID (*KernelEntry)(struct zev_boot_info *) =
        (VOID (*)(struct zev_boot_info *))(UINTN)Entry;
    __asm__ volatile("mov %0, %%r12" : : "r"(Info) : "r12");
    KernelEntry(Info);

    return EFI_SUCCESS;
}

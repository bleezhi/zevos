/* ZevBoot UEFI foundation - EDK2 application. */
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/GraphicsOutput.h>
#include <Guid/FileInfo.h>

#define ZEVBOOT_MAGIC 0x5A425446ULL /* "ZBTF" */

typedef struct {
    UINT64 magic;
    UINT64 version;
    UINT64 memory_map;
    UINT64 memory_map_size;
    UINT64 memory_map_desc_size;
    UINT64 framebuffer;
    UINT32 framebuffer_width;
    UINT32 framebuffer_height;
    UINT32 framebuffer_pitch;
    UINT32 framebuffer_format;
} ZEV_BOOT_INFO;

/* This foundation deliberately stops before ExitBootServices. The next boot
 * stage will add an ELF64 loader and a small architecture-specific handoff. */
EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS Status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop = NULL;
    ZEV_BOOT_INFO Info;

    ZeroMem(&Info, sizeof(Info));
    Info.magic = ZEVBOOT_MAGIC;
    Info.version = 1;

    Status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **)&Gop);
    if (!EFI_ERROR(Status) && Gop && Gop->Mode) {
        Info.framebuffer = (UINT64)Gop->Mode->FrameBufferBase;
        Info.framebuffer_width = Gop->Mode->Info->HorizontalResolution;
        Info.framebuffer_height = Gop->Mode->Info->VerticalResolution;
        Info.framebuffer_pitch = Gop->Mode->Info->PixelsPerScanLine * 4;
        Info.framebuffer_format = (UINT32)Gop->Mode->Info->PixelFormat;
    }

    Print(L"ZevBoot UEFI foundation\r\n");
    if (Gop)
        Print(L"GOP: %ux%u framebuffer=%lx\r\n", Info.framebuffer_width,
              Info.framebuffer_height, Info.framebuffer);
    Print(L"Next stage: ELF64 kernel loader + ExitBootServices handoff\r\n");
    return EFI_SUCCESS;
}

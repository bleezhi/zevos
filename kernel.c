/* ZevOS kernel - freestanding C */

#define VGA_MEMORY ((volatile unsigned short *)0xB8000)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

static void vga_clear(void)
{
    volatile unsigned short *vga = VGA_MEMORY;
    for (unsigned int i = 0; i < VGA_WIDTH * VGA_HEIGHT; ++i)
        vga[i] = 0x0F20; /* white on black, space */
}

void kernel_main(unsigned int multiboot_magic, unsigned int multiboot_info)
{
    (void)multiboot_magic;
    (void)multiboot_info;

    vga_clear();

    const char *message = "ZevOS booted successfully!";
    volatile unsigned short *vga = VGA_MEMORY;

    for (unsigned int i = 0; message[i] != '\0'; ++i)
        vga[i] = (unsigned short)message[i] | 0x0F00;

    for (;;)
        __asm__ volatile ("hlt");
}

/* ZevOS kernel - freestanding C */

#define VGA_MEMORY ((volatile unsigned short *)0xB8000)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

void keyboard_init(void);
void keyboard_poll(void);
void shell_init(void);

static void vga_clear(void)
{
    volatile unsigned short *vga = VGA_MEMORY;
    for (unsigned int i = 0; i < VGA_WIDTH * VGA_HEIGHT; ++i)
        vga[i] = 0x0F20;
}

void kernel_main(unsigned int multiboot_magic, unsigned int multiboot_info)
{
    (void)multiboot_magic;
    (void)multiboot_info;

    vga_clear();
    keyboard_init();
    shell_init();

    for (;;) {
        keyboard_poll();
        __asm__ volatile ("pause");
    }
}

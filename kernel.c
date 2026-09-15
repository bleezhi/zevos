/* ZevOS kernel - freestanding C */

void terminal_init(void);
void terminal_puts(const char *s);
void idt_init(void);
void keyboard_init(void);
void shell_init(void);
void pmm_init(unsigned int multiboot_info);
void process_init(void);
void zinit_init(void);

void kernel_main(unsigned int multiboot_magic, unsigned int multiboot_info)
{
    terminal_init();
    terminal_puts("ZevOS kernel starting...\n");

    if (multiboot_magic != 0x36D76289) {
        terminal_puts("WARNING: invalid Multiboot2 magic\n");
    }

    pmm_init(multiboot_info);
    terminal_puts("memory: physical page allocator ready\n");

    process_init();
    terminal_puts("process: scheduler foundation ready\n");

    idt_init();
    terminal_puts("interrupts: IDT/PIC/PIT ready\n");

    keyboard_init();
    zinit_init();
    shell_init();

    terminal_puts("ZevOS: kernel foundation ready\n");

    __asm__ volatile ("sti");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

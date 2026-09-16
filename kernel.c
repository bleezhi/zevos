/* ZevOS kernel - freestanding C */

void terminal_init(void);
void terminal_puts(const char *s);
void idt_init(void);
void keyboard_init(void);
void shell_init(void);
void pmm_init(unsigned int multiboot_info);
void process_init(void);
void zinit_init(void);
void vmm_init(void);
struct process;
struct process *process_create_first_user(void);
void process_launch_user(struct process *process);

void kernel_main(unsigned int multiboot_magic, unsigned int multiboot_info)
{
    terminal_init();

    terminal_puts("========================================\n");
    terminal_puts("          ZevOS NIGHTLY BUILD\n");
    terminal_puts("========================================\n");
    terminal_puts("WARNING: This is a development build.\n");
    terminal_puts("It may be unstable, incomplete, or broken.\n");
    terminal_puts("If you are a normal user, you should NOT use this build.\n");
    terminal_puts("Use a stable ZevOS release instead.\n");
    terminal_puts("========================================\n\n");

    terminal_puts("ZevOS kernel starting...\n");

    if (multiboot_magic != 0x36D76289) {
        terminal_puts("WARNING: invalid Multiboot2 magic\n");
    }

    pmm_init(multiboot_info);
    terminal_puts("memory: physical page allocator ready\n");

    vmm_init();
    terminal_puts("memory: virtual memory active\n");

    process_init();
    terminal_puts("process: scheduler foundation ready\n");
    terminal_puts("process: context-switch foundation ready\n");
    terminal_puts("usermode: ring 3 transition foundation ready\n");
    terminal_puts("tss: ring 3 interrupt stack foundation ready\n");

    idt_init();
    terminal_puts("interrupts: IDT/PIC/PIT ready\n");

    keyboard_init();
    zinit_init();
    shell_init();

    terminal_puts("ZevOS: kernel foundation ready\n");

    /* One combined smoke test for the userspace stack, code mapping,
     * initial iret frame, CR3 switch, ring-3 entry, and int 0x80 syscall. */
    struct process *first_user = process_create_first_user();
    if (first_user) {
        terminal_puts("userspace: first process image ready\n");
        terminal_puts("userspace: launching ring 3 test program...\n");
        process_launch_user(first_user);
    }

    terminal_puts("userspace: failed to create first process\n");

    __asm__ volatile ("sti");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

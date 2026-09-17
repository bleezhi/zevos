/* ZevOS kernel - freestanding C */

#include <stdint.h>

void terminal_init(void);
void terminal_puts(const char *s);
void idt_init(void);
void keyboard_init(void);
uint32_t input_activity_seen(void);
uint64_t timer_ticks(void);
void shell_init(void);
void pmm_init(unsigned int multiboot_info);
void process_init(void);
void zinit_init(void);
void vmm_init(void);
void vfs_init(void);
void fd_init(void);
struct process;
struct process *process_create_first_user_named(const char *name);
uint32_t process_first_user_error(void);
void process_launch_user(struct process *process);

static void print_digit(uint32_t value)
{
    char s[2];
    s[0] = (char)('0' + (value % 10));
    s[1] = 0;
    terminal_puts(s);
}

static void wait_for_boot_input_or_timeout(void)
{
    const uint64_t start = timer_ticks();
    const uint64_t timeout = 200; /* PIT is configured for ~100 Hz. */

    terminal_puts("boot: waiting 2 seconds for PS/2/USB input...\n");
    __asm__ volatile ("sti");

    while ((timer_ticks() - start) < timeout) {
        if (input_activity_seen()) {
            terminal_puts("boot: input detected, staying in shell\n");
            return;
        }
        __asm__ volatile ("hlt");
    }

    if (!input_activity_seen())
        terminal_puts("boot: no input detected, launching dsplayed\n");
}

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

    if (multiboot_magic != 0x36D76289)
        terminal_puts("WARNING: invalid Multiboot2 magic\n");

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

    vfs_init();
    terminal_puts("vfs: ramfs mounted at /\n");
    fd_init();
    terminal_puts("fd: stdin/stdout/stderr ready\n");

    zinit_init();
    shell_init();
    terminal_puts("ZevOS: userspace bootstrap ready\n");

    wait_for_boot_input_or_timeout();

    /* dsplayed is the first display-manager userspace slot. Its executable
     * is still the bootstrap ELF for now; the real display-server program
     * will replace it once the graphics/userspace stack is ready. */
    if (!input_activity_seen()) {
        struct process *display_manager =
            process_create_first_user_named("dsplayed");
        if (display_manager) {
            terminal_puts("userspace: dsplayed process ready\n");
            process_launch_user(display_manager);
        }

        terminal_puts("userspace: failed to create dsplayed (stage ");
        print_digit(process_first_user_error());
        terminal_puts(")\n");
    }

    __asm__ volatile ("sti");
    for (;;) __asm__ volatile ("hlt");
}

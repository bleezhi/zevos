/* ZevOS kernel - native ZevBoot handoff. */
#include <stdint.h>
#include "zevboot.h"

void terminal_init(const struct zev_boot_info *boot_info);
void terminal_puts(const char *s);
void idt_init(void);
void keyboard_init(void);
uint32_t input_activity_seen(void);
uint64_t timer_ticks(void);
void shell_init(void);
void pmm_init(const struct zev_boot_info *boot_info);
void process_init(void);
void zinit_init(void);
void vmm_init(void);
void vfs_init(void);
void fd_init(void);

struct process;
struct process *process_create_from_path(const char *path);
uint32_t process_first_user_error(void);
void process_launch_user(struct process *process);

static void print_digit(uint32_t v)
{
    char s[2];
    s[0] = (char)('0' + (v % 10));
    s[1] = 0;
    terminal_puts(s);
}

static void wait_for_boot_input_or_timeout(void)
{
    const uint64_t start = timer_ticks();
    const uint64_t timeout = 200;

    terminal_puts("boot: waiting 2 seconds for PS/2/USB input...\n");
    __asm__ volatile("sti");

    while ((timer_ticks() - start) < timeout) {
        if (input_activity_seen()) {
            terminal_puts("boot: input detected, staying in shell\n");
            return;
        }
        __asm__ volatile("hlt");
    }

    if (!input_activity_seen())
        terminal_puts("boot: no input detected, launching dsplayed\n");
}

void kernel_main(struct zev_boot_info *boot_info)
{
    terminal_init(boot_info);

    terminal_puts("========================================\n");
    terminal_puts("          ZevOS NIGHTLY BUILD\n");
    terminal_puts("========================================\n");
    terminal_puts("Native ZevBoot is active.\n");
    terminal_puts("========================================\n\n");
    terminal_puts("ZevOS kernel starting...\n");

    if (!boot_info || boot_info->magic != ZEV_BOOT_MAGIC) {
        terminal_puts("FATAL: invalid ZevBoot handoff\n");
        for (;;) __asm__ volatile("cli\n\thlt");
    }

    pmm_init(boot_info);
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

    if (!input_activity_seen()) {
        struct process *display_manager =
            process_create_from_path("/bin/dsplayed");

        if (display_manager) {
            terminal_puts("userspace: loaded /bin/dsplayed from VFS\n");
            terminal_puts("userspace: launching dsplayed\n");
            process_launch_user(display_manager);
        }

        terminal_puts("userspace: failed to load /bin/dsplayed (stage ");
        print_digit(process_first_user_error());
        terminal_puts(")\n");
    }

    __asm__ volatile("sti");
    for (;;) __asm__ volatile("hlt");
}

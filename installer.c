/* ZevOS native ZevBoot installer. */
#include <stdint.h>
#include "vfs.h"
#include "zevfs.h"
#include "ata.h"

void terminal_clear(void);
void terminal_puts(const char *s);
void terminal_set_color(uint8_t color);
void shell_init(void);

extern const uint8_t zevboot_stage1_start[];
extern const uint8_t zevboot_stage1_end[];
extern const uint8_t zevboot_stage2_start[];
extern const uint8_t zevboot_stage2_end[];
extern const uint8_t kernel_start[];
extern const uint8_t kernel_end[];

static int active, installed, formatted, boot_installed;
static int selected_format = 1, selected_copy = 1, selected_boot = 1;

static void yn_line(const char *text, int value)
{
    terminal_puts("                         ");
    terminal_puts(text);
    terminal_puts(" [");
    terminal_puts(value ? "Y" : "N");
    terminal_puts("]\n");
}

static void draw(void)
{
    terminal_set_color(0x1F);
    terminal_clear();
    terminal_puts("\n                         [ 1 ] ZevOS installer\n\n");
    terminal_puts("                         Target: HDA0 (IDE primary master)\n\n");
    terminal_puts("                         Installation options:\n\n");
    yn_line("Format HDA0 as ZevFS?", selected_format);
    yn_line("Install /bin /etc /usr /zev?", selected_copy);
    yn_line("Install ZevBoot + kernel?", selected_boot);
    terminal_puts("\n                         Y = yes   N = no\n");
    terminal_puts("                         1-3 select option   I = install   M = main menu\n\n");
    if (formatted)
        terminal_puts("                         [OK] ZevFS formatted on HDA0\n");
    if (installed)
        terminal_puts("                         [OK] ZevOS files written to HDA0\n");
    if (boot_installed)
        terminal_puts("                         [OK] ZevBoot BIOS + kernel installed\n");
}

static int put(const char *path)
{
    uint8_t data[512];
    int n = vfs_read_binary(path, data, sizeof(data));
    if (n < 0) return -1;
    return zevfs_write_file(path, data, (uint32_t)n);
}

static int install_files(void)
{
    const char *files[] = {
        "/etc/hostname", "/etc/motd", "/zev/release",
        "/bin/echo", "/bin/cat", "/bin/ls", "/bin/pwd",
        "/bin/true", "/bin/false", "/bin/zinit", "/bin/dsplayed"
    };

    for (unsigned int i = 0; i < sizeof(files) / sizeof(files[0]); ++i)
        if (put(files[i])) return -1;

    return 0;
}

static void le32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static int install_zevboot(void)
{
    uint8_t sector[512];
    uint32_t stage1_size =
        (uint32_t)(zevboot_stage1_end - zevboot_stage1_start);
    uint32_t stage2_size =
        (uint32_t)(zevboot_stage2_end - zevboot_stage2_start);
    uint32_t kernel_bytes =
        (uint32_t)(kernel_end - kernel_start);
    uint32_t kernel_sectors = (kernel_bytes + 511u) / 512u;

    if (stage1_size != 512u || stage2_size != 16u * 512u ||
        !kernel_sectors || kernel_sectors > 1792u)
        return -1;

    for (int i = 0; i < 512; ++i)
        sector[i] = zevboot_stage1_start[i];
    if (ata_write28(0, sector)) return -1;

    for (int i = 0; i < 512; ++i) sector[i] = 0;
    sector[0] = 'Z'; sector[1] = 'B'; sector[2] = 'O'; sector[3] = 'T';
    le32(sector + 4, kernel_sectors);
    le32(sector + 8, kernel_bytes);
    le32(sector + 12, 18u);
    if (ata_write28(1, sector)) return -1;

    for (uint32_t s = 0; s < 16; ++s) {
        for (int j = 0; j < 512; ++j)
            sector[j] = zevboot_stage2_start[s * 512u + (uint32_t)j];
        if (ata_write28(2u + s, sector)) return -1;
    }

    const uint8_t *src = kernel_start;
    for (uint32_t s = 0; s < kernel_sectors; ++s) {
        for (int j = 0; j < 512; ++j) {
            uint32_t off = s * 512u + (uint32_t)j;
            sector[j] = off < kernel_bytes ? src[off] : 0;
        }
        if (ata_write28(18u + s, sector)) return -1;
    }

    return 0;
}

static void do_install(void)
{
    if (ata_init()) {
        terminal_puts("\n                         ERROR: HDA0 not detected.\n");
        return;
    }

    if (selected_format) {
        if (zevfs_format()) {
            terminal_puts("\n                         ERROR: HDA0 format failed.\n");
            return;
        }
        formatted = 1;
    } else if (!zevfs_ready() && zevfs_mount()) {
        terminal_puts("\n                         ERROR: HDA0 has no ZevFS filesystem.\n");
        return;
    }

    if (selected_copy) {
        if (install_files()) {
            terminal_puts("\n                         ERROR: could not write ZevOS files.\n");
            return;
        }
        installed = 1;
    }

    if (selected_boot) {
        if (install_zevboot()) {
            terminal_puts("\n                         ERROR: ZevBoot installation failed.\n");
            return;
        }
        boot_installed = 1;
    }

    draw();
}

void installer_start(void)
{
    active = 1;
    installed = 0;
    formatted = 0;
    boot_installed = 0;
    selected_format = selected_copy = selected_boot = 1;
    draw();
}

int installer_active(void) { return active; }

void installer_input(char c)
{
    if (!active) return;

    if (c == '1') { selected_format = !selected_format; draw(); return; }
    if (c == '2') { selected_copy = !selected_copy; draw(); return; }
    if (c == '3') { selected_boot = !selected_boot; draw(); return; }

    if (c == 'y' || c == 'Y') {
        selected_format = selected_copy = selected_boot = 1;
        draw();
        return;
    }

    if (c == 'n' || c == 'N') {
        selected_format = selected_copy = selected_boot = 0;
        draw();
        return;
    }

    if (c == 'i' || c == 'I') { do_install(); return; }

    if (c == 'm' || c == 'M') {
        active = 0;
        terminal_set_color(0x0F);
        terminal_clear();
        shell_init();
    }
}

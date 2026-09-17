/* ZevOS Stage 0 text-mode installer. */
#include <stdint.h>
#include "vfs.h"

void terminal_clear(void);
void terminal_puts(const char *s);
void terminal_putchar(char c);
void terminal_set_color(uint8_t color);
void shell_init(void);

static int active;
static int installed;

static void installer_draw(void)
{
    terminal_set_color(0x5F); /* bright white on purple */
    terminal_clear();
    terminal_puts("                                                                                \n");
    terminal_puts("                         ZevOS INSTALLER - STAGE 0                            \n");
    terminal_puts("                                                                                \n");
    terminal_puts("                         [ I ] Install ZevOS                                   \n");
    terminal_puts("                         [ Q ] Return to shell                                \n");
    terminal_puts("                                                                                \n");
    terminal_puts(" Target: HDA0                                                               \n");
    terminal_puts(" Layout: /bin /etc /home /usr /var /zev                                     \n");
    terminal_puts("                                                                                \n");
    terminal_puts(" Stage 0 prepares the ZevOS filesystem layout in the live VFS.               \n");
    terminal_puts(" Persistent HDA writing will be enabled by the disk filesystem layer.      \n");
    terminal_puts("                                                                                \n");
    if (installed)
        terminal_puts(" Installation layout prepared. Press Q to return.                           \n");
    else
        terminal_puts(" Press I to begin.                                                          \n");
}

static void prepare_layout(void)
{
    vfs_mkdir("/bin"); vfs_mkdir("/dev"); vfs_mkdir("/etc");
    vfs_mkdir("/home"); vfs_mkdir("/tmp"); vfs_mkdir("/usr");
    vfs_mkdir("/var"); vfs_mkdir("/usr/bin"); vfs_mkdir("/usr/lib");
    vfs_mkdir("/zev");
    if (!vfs_exists("/etc/hostname")) vfs_touch("/etc/hostname");
    vfs_write("/etc/hostname", "ZevOS\n", 6);
    if (!vfs_exists("/etc/motd")) vfs_touch("/etc/motd");
    vfs_write("/etc/motd", "Welcome to ZevOS.\n", 19);
    if (!vfs_exists("/zev/release")) vfs_touch("/zev/release");
    vfs_write("/zev/release", "ZevOS Stage 0\n", 14);
    installed = 1;
}

void installer_start(void)
{
    active = 1;
    installed = 0;
    installer_draw();
}

int installer_active(void)
{
    return active;
}

void installer_input(char c)
{
    if (!active) return;
    if (c == 'i' || c == 'I') {
        prepare_layout();
        installer_draw();
        return;
    }
    if (c == 'q' || c == 'Q') {
        active = 0;
        installed = 0;
        terminal_set_color(0x0F);
        terminal_clear();
        shell_init();
    }
}

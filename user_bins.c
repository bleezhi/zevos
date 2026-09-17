/* Built-in ELF64 /bin programs for the initial ramfs. */
#include <stdint.h>
#include "vfs.h"

/* Tiny ET_EXEC images. Each is a real ELF64 file with one PT_LOAD segment.
 * The programs use ZevOS int 0x80 SYS_WRITE/SYS_EXIT and are intentionally
 * small enough for the current ramfs node size. */

static const uint8_t bin_echo[] = {
#include "user_bins_echo.inc"
};

static const uint8_t bin_cat[] = {
#include "user_bins_cat.inc"
};

static const uint8_t bin_ls[] = {
#include "user_bins_ls.inc"
};

static const uint8_t bin_pwd[] = {
#include "user_bins_pwd.inc"
};

static const uint8_t bin_true[] = {
#include "user_bins_true.inc"
};

static const uint8_t bin_false[] = {
#include "user_bins_false.inc"
};

static const uint8_t bin_zinit[] = {
#include "user_bins_zinit.inc"
};

void vfs_install_binaries(void)
{
    vfs_write_binary("/bin/echo", bin_echo, sizeof(bin_echo));
    vfs_write_binary("/bin/cat", bin_cat, sizeof(bin_cat));
    vfs_write_binary("/bin/ls", bin_ls, sizeof(bin_ls));
    vfs_write_binary("/bin/pwd", bin_pwd, sizeof(bin_pwd));
    vfs_write_binary("/bin/true", bin_true, sizeof(bin_true));
    vfs_write_binary("/bin/false", bin_false, sizeof(bin_false));
    vfs_write_binary("/bin/zinit", bin_zinit, sizeof(bin_zinit));
    vfs_write_binary("/bin/dsplayed", bin_zinit, sizeof(bin_zinit));
}

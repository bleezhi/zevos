/* ZevOS bootstrap file descriptor layer. */
#include <stdint.h>
#include "vfs.h"

#define FD_MAX 32
#define FD_UNUSED 0
#define FD_FILE 1
#define FD_STDIN 2
#define FD_STDOUT 3
#define FD_STDERR 4

#define USER_MIN 0x400000ULL
#define USER_MAX 0x800000ULL

struct fd_entry {
    uint8_t state;
    char path[128];
};

static struct fd_entry fds[FD_MAX];
extern void terminal_putchar(char c);

static uint64_t user_range_ok(uint64_t ptr, uint64_t count)
{
    if (ptr < USER_MIN || ptr >= USER_MAX)
        return 0;
    if (count > USER_MAX - ptr)
        return 0;
    return 1;
}

static uint64_t user_ptr_ok(uint64_t ptr)
{
    return user_range_ok(ptr, 1);
}

void fd_init(void)
{
    for (unsigned int i = 0; i < FD_MAX; ++i) {
        fds[i].state = FD_UNUSED;
        fds[i].path[0] = 0;
    }
    fds[0].state = FD_STDIN;
    fds[1].state = FD_STDOUT;
    fds[2].state = FD_STDERR;
}

static int copy_user_string(char *dst, uint64_t src, unsigned int max)
{
    if (!dst || !user_ptr_ok(src) || max < 2)
        return -1;
    const char *s = (const char *)src;
    for (unsigned int i = 0; i + 1 < max; ++i) {
        if (!user_ptr_ok(src + i))
            return -1;
        dst[i] = s[i];
        if (dst[i] == 0)
            return 0;
    }
    dst[max - 1] = 0;
    return -1;
}

int fd_open(uint64_t user_path)
{
    char path[128];
    if (copy_user_string(path, user_path, sizeof(path)) != 0 || !vfs_exists(path))
        return -1;

    for (int i = 3; i < FD_MAX; ++i) {
        if (fds[i].state == FD_UNUSED) {
            unsigned int n = 0;
            while (path[n] && n + 1 < sizeof(fds[i].path)) {
                fds[i].path[n] = path[n];
                ++n;
            }
            fds[i].path[n] = 0;
            fds[i].state = FD_FILE;
            return i;
        }
    }
    return -1;
}

int fd_close(int fd)
{
    if (fd < 3 || fd >= FD_MAX || fds[fd].state != FD_FILE)
        return -1;
    fds[fd].state = FD_UNUSED;
    fds[fd].path[0] = 0;
    return 0;
}

int fd_write(int fd, uint64_t user_buffer, uint64_t count)
{
    if (fd != 1 && fd != 2)
        return -1;
    if (count > 4096 || !user_range_ok(user_buffer, count))
        return -1;

    const char *buffer = (const char *)user_buffer;
    for (uint64_t i = 0; i < count; ++i)
        terminal_putchar(buffer[i]);
    return (int)count;
}

int fd_read(int fd, uint64_t user_buffer, uint64_t count)
{
    if (fd < 3 || fd >= FD_MAX || fds[fd].state != FD_FILE ||
        count == 0 || count > 512 || !user_range_ok(user_buffer, count))
        return -1;

    char temp[513];
    int result = vfs_read(fds[fd].path, temp, count + 1);
    if (result < 0)
        return -1;

    char *dst = (char *)user_buffer;
    for (int i = 0; i < result; ++i)
        dst[i] = temp[i];
    return result;
}

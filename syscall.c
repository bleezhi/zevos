/* ZevOS early userspace syscall dispatcher. */

#include <stdint.h>

int fd_open(uint64_t user_path);
int fd_close(int fd);
int fd_read(int fd, uint64_t user_buffer, uint64_t count);
int fd_write(int fd, uint64_t user_buffer, uint64_t count);

uint64_t syscall_dispatch(uint64_t number, uint64_t arg1,
                          uint64_t arg2, uint64_t arg3)
{
    switch (number) {
    case 0: /* read(fd, buffer, count) */
        return (uint64_t)fd_read((int)arg1, arg2, arg3);
    case 1: /* write(fd, buffer, count) */
        return (uint64_t)fd_write((int)arg1, arg2, arg3);
    case 2: /* open(path, flags) -- flags are currently ignored */
        (void)arg2;
        return (uint64_t)fd_open(arg1);
    case 3: /* exit(status) -- process teardown is the next scheduler step */
        (void)arg1;
        return 0;
    case 4: /* close(fd) */
        return (uint64_t)fd_close((int)arg1);
    default:
        return (uint64_t)-1;
    }
}

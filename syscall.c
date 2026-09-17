/* ZevOS userspace syscall ABI. */

#include <stdint.h>

/* ABI: RAX = syscall number, RDI/RSI/RDX = arguments 1/2/3. */
enum {
    SYS_READ  = 0,
    SYS_WRITE = 1,
    SYS_OPEN  = 2,
    SYS_EXIT  = 3,
    SYS_CLOSE = 4
};

#define SYSCALL_ERROR ((uint64_t)-1)

int fd_open(uint64_t user_path);
int fd_close(int fd);
int fd_read(int fd, uint64_t user_buffer, uint64_t count);
int fd_write(int fd, uint64_t user_buffer, uint64_t count);
__attribute__((noreturn)) void process_exit_current(uint64_t status);

uint64_t syscall_dispatch(uint64_t number, uint64_t arg1,
                          uint64_t arg2, uint64_t arg3)
{
    switch (number) {
    case SYS_READ:
        return (uint64_t)fd_read((int)arg1, arg2, arg3);

    case SYS_WRITE:
        return (uint64_t)fd_write((int)arg1, arg2, arg3);

    case SYS_OPEN:
        /* Flags are intentionally ignored in this bootstrap ABI. */
        (void)arg2;
        return (uint64_t)fd_open(arg1);

    case SYS_EXIT:
        process_exit_current(arg1);
        __builtin_unreachable();

    case SYS_CLOSE:
        return (uint64_t)fd_close((int)arg1);

    default:
        return SYSCALL_ERROR;
    }
}

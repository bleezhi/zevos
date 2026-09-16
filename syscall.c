/* ZevOS early userspace syscall dispatcher. */

#include <stdint.h>

void terminal_puts(const char *s);

uint64_t syscall_dispatch(uint64_t number, uint64_t arg1, uint64_t arg2)
{
    (void)arg2;

    switch (number) {
    case 0:
        return 0;

    case 1:
        /* Early write syscall: arg1 is a NUL-terminated user string. */
        if (arg1 >= 0x400000ULL && arg1 < 0x800000ULL)
            terminal_puts((const char *)arg1);
        return 0;

    default:
        return (uint64_t)-1;
    }
}

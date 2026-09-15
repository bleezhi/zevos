/* ZevOS early syscall dispatcher. */

#include <stdint.h>

uint64_t syscall_dispatch(uint64_t number)
{
    switch (number) {
    case 0:
        return 0;
    default:
        return (uint64_t)-1;
    }
}

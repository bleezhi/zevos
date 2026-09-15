/* ZevOS user-mode transition foundation. */

#include <stdint.h>

/* GDT selectors installed by boot.asm. RPL=3 selects ring 3. */
#define USER_CODE_SELECTOR 0x1B
#define USER_DATA_SELECTOR 0x23

extern void user_enter(uint64_t entry, uint64_t stack,
                       uint64_t user_code, uint64_t user_data);

/* Enter ring 3 once a process has a user-mapped address space.
 * The current kernel does not call this until the VM/ELF loader is ready. */
void usermode_enter(uint64_t entry, uint64_t stack)
{
    user_enter(entry, stack, USER_CODE_SELECTOR, USER_DATA_SELECTOR);
}

uint16_t user_code_selector(void)
{
    return USER_CODE_SELECTOR;
}

uint16_t user_data_selector(void)
{
    return USER_DATA_SELECTOR;
}

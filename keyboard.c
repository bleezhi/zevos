/* ZevOS PS/2 keyboard driver */

#include <stdint.h>

#define KEYBOARD_DATA 0x60
#define KEYBOARD_STATUS 0x64

void shell_input(char c);

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static const char keys[] = "\0\0" "1234567890-=\b\t" "qwertyuiop[]\n" "\0asdfghjkl;'`" "\0\\zxcvbnm,./";

void keyboard_init(void)
{
    /* IRQ1 is enabled by the PIC during interrupt initialization. */
}

void keyboard_irq(void)
{
    while (inb(KEYBOARD_STATUS) & 1) {
        uint8_t sc = inb(KEYBOARD_DATA);
        if (sc & 0x80)
            continue;

        /* PS/2 set 1 scancode 0x39 is Space. */
        if (sc == 0x39) {
            shell_input(' ');
        } else if (sc < sizeof(keys) - 1 && keys[sc]) {
            shell_input(keys[sc]);
        }
    }
}

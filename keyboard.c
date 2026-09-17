/* ZevOS PS/2 keyboard driver */

#include <stdint.h>

#define KEYBOARD_DATA 0x60
#define KEYBOARD_STATUS 0x64

void shell_input(char c);
int installer_active(void);
void installer_input(char c);

static volatile uint32_t input_activity;

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static const char keys[] = "\0\0" "1234567890-=\b\t" "qwertyuiop[]\n" "\0asdfghjkl;'`" "\0\\zxcvbnm,./";

static void deliver_key(char c)
{
    if (installer_active())
        installer_input(c);
    else
        shell_input(c);
}

void keyboard_init(void)
{
    input_activity = 0;
}

void keyboard_irq(void)
{
    while (inb(KEYBOARD_STATUS) & 1) {
        uint8_t sc = inb(KEYBOARD_DATA);
        if (sc & 0x80)
            continue;

        input_activity = 1;

        /* PS/2 set 1 scancode 0x39 is Space. */
        if (sc == 0x39) {
            deliver_key(' ');
        } else if (sc < sizeof(keys) - 1 && keys[sc]) {
            deliver_key(keys[sc]);
        }
    }
}

void input_activity_mark(void)
{
    input_activity = 1;
}

uint32_t input_activity_seen(void)
{
    return input_activity;
}

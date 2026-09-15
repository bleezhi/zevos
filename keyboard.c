/* ZevOS PS/2 keyboard driver */

#define VGA ((volatile unsigned short *)0xB8000)
#define DATA 0x60
#define STATUS 0x64
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

static unsigned int cursor = 0;

void shell_input(char c);

static unsigned char inb(unsigned short port)
{
    unsigned char value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void terminal_putchar(char c)
{
    if (c == '\n') {
        cursor = (cursor / VGA_WIDTH + 1) * VGA_WIDTH;
    } else {
        if (cursor >= VGA_WIDTH * VGA_HEIGHT)
            cursor = (VGA_HEIGHT - 1) * VGA_WIDTH;
        VGA[cursor++] = (unsigned short)c | 0x0F00;
    }

    if (cursor >= VGA_WIDTH * VGA_HEIGHT)
        cursor = (VGA_HEIGHT - 1) * VGA_WIDTH;
}

void terminal_puts(const char *s)
{
    while (*s)
        terminal_putchar(*s++);
}

void terminal_backspace(void)
{
    if (cursor > 0) {
        --cursor;
        VGA[cursor] = 0x0F20;
    }
}

void terminal_clear(void)
{
    for (unsigned int i = 0; i < VGA_WIDTH * VGA_HEIGHT; ++i)
        VGA[i] = 0x0F20;
    cursor = 0;
}

static const char keys[] = "\0\0" "1234567890-=\b\t" "qwertyuiop[]\n" "\0asdfghjkl;'`" "\0\\zxcvbnm,./";

void keyboard_init(void)
{
    /* The shell owns the terminal banner. */
}

void keyboard_poll(void)
{
    while (inb(STATUS) & 1) {
        unsigned char sc = inb(DATA);

        /* Ignore key-release scancodes for now. */
        if (sc & 0x80)
            continue;

        if (sc < sizeof(keys) - 1 && keys[sc])
            shell_input(keys[sc]);
    }
}

#define VGA ((volatile unsigned short *)0xB8000)
#define DATA 0x60
#define STATUS 0x64

static unsigned int cursor = 0;

static unsigned char inb(unsigned short port)
{
    unsigned char value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static const char keys[] = "\0\0" "1234567890-=\b\t" "qwertyuiop[]\n" "\0asdfghjkl;'`" "\0\\zxcvbnm,./";

static void putchar(char c)
{
    if (c == '\n') { cursor = (cursor / 80 + 1) * 80; return; }
    if (c == '\b') { if (cursor) { --cursor; VGA[cursor] = 0x0F20; } return; }
    if (cursor < 2000) VGA[cursor++] = (unsigned short)c | 0x0F00;
}

void keyboard_init(void)
{
    const char *s = "ZevOS v0.1\nPS/2 keyboard ready. Type something!\n> ";
    while (*s) putchar(*s++);
}

void keyboard_poll(void)
{
    while (inb(STATUS) & 1) {
        unsigned char sc = inb(DATA);
        if (sc & 0x80) continue;
        if (sc < sizeof(keys) - 1 && keys[sc]) putchar(keys[sc]);
    }
}

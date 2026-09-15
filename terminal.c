/* ZevOS terminal subsystem */

#include <stdint.h>

#define VGA_MEMORY ((volatile uint16_t *)0xB8000)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

static unsigned int row;
static unsigned int column;
static uint8_t color = 0x0F;

static void move_cursor(void)
{
    unsigned short position = (unsigned short)(row * VGA_WIDTH + column);
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x0F), "Nd"((uint16_t)0x3D4));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)(position & 0xFF)), "Nd"((uint16_t)0x3D5));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x0E), "Nd"((uint16_t)0x3D4));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)(position >> 8)), "Nd"((uint16_t)0x3D5));
}

static void scroll(void)
{
    volatile uint16_t *vga = VGA_MEMORY;
    for (unsigned int y = 1; y < VGA_HEIGHT; ++y)
        for (unsigned int x = 0; x < VGA_WIDTH; ++x)
            vga[(y - 1) * VGA_WIDTH + x] = vga[y * VGA_WIDTH + x];

    for (unsigned int x = 0; x < VGA_WIDTH; ++x)
        vga[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = ((uint16_t)color << 8) | ' ';

    row = VGA_HEIGHT - 1;
}

void terminal_clear(void)
{
    volatile uint16_t *vga = VGA_MEMORY;
    for (unsigned int i = 0; i < VGA_WIDTH * VGA_HEIGHT; ++i)
        vga[i] = ((uint16_t)color << 8) | ' ';
    row = 0;
    column = 0;
    move_cursor();
}

void terminal_putchar(char c)
{
    volatile uint16_t *vga = VGA_MEMORY;

    if (c == '\n') {
        column = 0;
        ++row;
    } else if (c == '\r') {
        column = 0;
    } else if (c == '\b') {
        if (column > 0) {
            --column;
            vga[row * VGA_WIDTH + column] = ((uint16_t)color << 8) | ' ';
        }
    } else {
        if (column >= VGA_WIDTH) {
            column = 0;
            ++row;
        }
        if (row >= VGA_HEIGHT)
            scroll();
        vga[row * VGA_WIDTH + column] = ((uint16_t)color << 8) | (uint8_t)c;
        ++column;
    }

    if (row >= VGA_HEIGHT)
        scroll();
    move_cursor();
}

void terminal_puts(const char *s)
{
    while (*s)
        terminal_putchar(*s++);
}

void terminal_init(void)
{
    terminal_clear();
}

void terminal_set_color(uint8_t new_color)
{
    color = new_color;
}

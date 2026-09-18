/* ZevOS terminal: VGA text on BIOS, simple GOP framebuffer on UEFI. */
#include <stdint.h>
#include "zevboot.h"

#define VGA_MEMORY ((volatile uint16_t *)0xB8000)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define FB_CELL_W 6
#define FB_CELL_H 8

static unsigned int row, column;
static uint8_t color = 0x0F;
static const struct zev_boot_info *boot_info;
static int fb_mode;

static uint32_t rgb(uint8_t index)
{
    static const uint8_t table[16][3] = {
        {0,0,0},{0,0,170},{0,170,0},{0,170,170},
        {170,0,0},{170,0,170},{170,85,0},{170,170,170},
        {85,85,85},{85,85,255},{85,255,85},{85,255,255},
        {255,85,85},{255,85,255},{255,255,85},{255,255,255}
    };
    return ((uint32_t)table[index & 15][0] << 16) |
           ((uint32_t)table[index & 15][1] << 8) |
           table[index & 15][2];
}

static uint32_t fb_color(uint8_t index)
{
    uint32_t c = rgb(index);
    uint8_t r = (uint8_t)(c >> 16);
    uint8_t g = (uint8_t)(c >> 8);
    uint8_t b = (uint8_t)c;

    if (boot_info->framebuffer_format == 1)
        return ((uint32_t)b << 16) | ((uint32_t)g << 8) | r;
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

static void fb_pixel(unsigned int x, unsigned int y, uint32_t value)
{
    volatile uint32_t *fb = (volatile uint32_t *)(uint64_t)boot_info->framebuffer;
    *(volatile uint32_t *)((volatile uint8_t *)fb +
        y * boot_info->framebuffer_pitch + x * 4) = value;
}

/* Compact 5x7 glyphs. Lowercase is displayed using the uppercase glyph. */
static void glyph(char ch, uint8_t g[7])
{
    if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');

    for (int i = 0; i < 7; ++i) g[i] = 0;

    switch (ch) {
    case 'A': g[0]=14;g[1]=17;g[2]=17;g[3]=31;g[4]=17;g[5]=17;g[6]=17;break;
    case 'B': g[0]=30;g[1]=17;g[2]=17;g[3]=30;g[4]=17;g[5]=17;g[6]=30;break;
    case 'C': g[0]=14;g[1]=17;g[2]=16;g[3]=16;g[4]=16;g[5]=17;g[6]=14;break;
    case 'D': g[0]=30;g[1]=17;g[2]=17;g[3]=17;g[4]=17;g[5]=17;g[6]=30;break;
    case 'E': g[0]=31;g[1]=16;g[2]=16;g[3]=30;g[4]=16;g[5]=16;g[6]=31;break;
    case 'F': g[0]=31;g[1]=16;g[2]=16;g[3]=30;g[4]=16;g[5]=16;g[6]=16;break;
    case 'G': g[0]=14;g[1]=17;g[2]=16;g[3]=23;g[4]=17;g[5]=17;g[6]=14;break;
    case 'H': g[0]=17;g[1]=17;g[2]=17;g[3]=31;g[4]=17;g[5]=17;g[6]=17;break;
    case 'I': g[0]=31;g[1]=4;g[2]=4;g[3]=4;g[4]=4;g[5]=4;g[6]=31;break;
    case 'J': g[0]=7;g[1]=2;g[2]=2;g[3]=2;g[4]=18;g[5]=18;g[6]=12;break;
    case 'K': g[0]=17;g[1]=18;g[2]=20;g[3]=24;g[4]=20;g[5]=18;g[6]=17;break;
    case 'L': g[0]=16;g[1]=16;g[2]=16;g[3]=16;g[4]=16;g[5]=16;g[6]=31;break;
    case 'M': g[0]=17;g[1]=27;g[2]=21;g[3]=21;g[4]=17;g[5]=17;g[6]=17;break;
    case 'N': g[0]=17;g[1]=25;g[2]=21;g[3]=19;g[4]=17;g[5]=17;g[6]=17;break;
    case 'O': g[0]=14;g[1]=17;g[2]=17;g[3]=17;g[4]=17;g[5]=17;g[6]=14;break;
    case 'P': g[0]=30;g[1]=17;g[2]=17;g[3]=30;g[4]=16;g[5]=16;g[6]=16;break;
    case 'Q': g[0]=14;g[1]=17;g[2]=17;g[3]=17;g[4]=21;g[5]=18;g[6]=13;break;
    case 'R': g[0]=30;g[1]=17;g[2]=17;g[3]=30;g[4]=20;g[5]=18;g[6]=17;break;
    case 'S': g[0]=15;g[1]=16;g[2]=16;g[3]=14;g[4]=1;g[5]=1;g[6]=30;break;
    case 'T': g[0]=31;g[1]=4;g[2]=4;g[3]=4;g[4]=4;g[5]=4;g[6]=4;break;
    case 'U': g[0]=17;g[1]=17;g[2]=17;g[3]=17;g[4]=17;g[5]=17;g[6]=14;break;
    case 'V': g[0]=17;g[1]=17;g[2]=17;g[3]=17;g[4]=10;g[5]=10;g[6]=4;break;
    case 'W': g[0]=17;g[1]=17;g[2]=17;g[3]=21;g[4]=21;g[5]=27;g[6]=17;break;
    case 'X': g[0]=17;g[1]=17;g[2]=10;g[3]=4;g[4]=10;g[5]=17;g[6]=17;break;
    case 'Y': g[0]=17;g[1]=17;g[2]=10;g[3]=4;g[4]=4;g[5]=4;g[6]=4;break;
    case 'Z': g[0]=31;g[1]=1;g[2]=2;g[3]=4;g[4]=8;g[5]=16;g[6]=31;break;
    case '0': g[0]=14;g[1]=17;g[2]=19;g[3]=21;g[4]=25;g[5]=17;g[6]=14;break;
    case '1': g[0]=4;g[1]=12;g[2]=4;g[3]=4;g[4]=4;g[5]=4;g[6]=14;break;
    case '2': g[0]=14;g[1]=17;g[2]=1;g[3]=2;g[4]=4;g[5]=8;g[6]=31;break;
    case '3': g[0]=30;g[1]=1;g[2]=1;g[3]=14;g[4]=1;g[5]=1;g[6]=30;break;
    case '4': g[0]=2;g[1]=6;g[2]=10;g[3]=18;g[4]=31;g[5]=2;g[6]=2;break;
    case '5': g[0]=31;g[1]=16;g[2]=16;g[3]=30;g[4]=1;g[5]=1;g[6]=30;break;
    case '6': g[0]=14;g[1]=16;g[2]=16;g[3]=30;g[4]=17;g[5]=17;g[6]=14;break;
    case '7': g[0]=31;g[1]=1;g[2]=2;g[3]=4;g[4]=8;g[5]=8;g[6]=8;break;
    case '8': g[0]=14;g[1]=17;g[2]=17;g[3]=14;g[4]=17;g[5]=17;g[6]=14;break;
    case '9': g[0]=14;g[1]=17;g[2]=17;g[3]=15;g[4]=1;g[5]=1;g[6]=14;break;
    case '!': g[0]=4;g[1]=4;g[2]=4;g[3]=4;g[4]=4;g[5]=0;g[6]=4;break;
    case '?': g[0]=14;g[1]=17;g[2]=1;g[3]=2;g[4]=4;g[5]=0;g[6]=4;break;
    case ':': g[2]=4;g[5]=4;break;
    case '.': g[6]=4;break;
    case ',': g[5]=4;g[6]=8;break;
    case '-': g[3]=14;break;
    case '_': g[6]=31;break;
    case '/': g[0]=1;g[1]=2;g[2]=2;g[3]=4;g[4]=8;g[5]=8;g[6]=16;break;
    case '\\': g[0]=16;g[1]=8;g[2]=8;g[3]=4;g[4]=2;g[5]=2;g[6]=1;break;
    case '[': g[0]=28;g[1]=16;g[2]=16;g[3]=16;g[4]=16;g[5]=16;g[6]=28;break;
    case ']': g[0]=7;g[1]=1;g[2]=1;g[3]=1;g[4]=1;g[5]=1;g[6]=7;break;
    case '(': g[0]=2;g[1]=4;g[2]=8;g[3]=8;g[4]=8;g[5]=4;g[6]=2;break;
    case ')': g[0]=8;g[1]=4;g[2]=2;g[3]=2;g[4]=2;g[5]=4;g[6]=8;break;
    case '+': g[3]=4;g[1]=4;g[2]=4;g[3]=31;g[4]=4;g[5]=4;break;
    case '=': g[2]=31;g[4]=31;break;
    case '|': g[0]=4;g[1]=4;g[2]=4;g[3]=4;g[4]=4;g[5]=4;g[6]=4;break;
    case '@': g[0]=14;g[1]=17;g[2]=23;g[3]=21;g[4]=23;g[5]=16;g[6]=14;break;
    case '#': g[1]=10;g[2]=31;g[3]=10;g[4]=31;g[5]=10;break;
    case '*': g[1]=4;g[2]=21;g[3]=14;g[4]=21;g[5]=4;break;
    case '"': g[0]=10;g[1]=10;break;
    case '\'': g[0]=4;g[1]=4;break;
    case '<': g[2]=4;g[3]=8;g[4]=4;break;
    case '>': g[2]=4;g[3]=2;g[4]=4;break;
    case ' ': break;
    default: g[0]=31;g[1]=17;g[2]=21;g[3]=21;g[4]=21;g[5]=17;g[6]=31;break;
    }
}

static void fb_fill(uint32_t value)
{
    for (unsigned int y = 0; y < boot_info->framebuffer_height; ++y)
        for (unsigned int x = 0; x < boot_info->framebuffer_width; ++x)
            fb_pixel(x, y, value);
}

static void fb_draw_char(unsigned int cx, unsigned int cy, char ch)
{
    uint8_t g[7];
    glyph(ch, g);
    uint32_t fg = fb_color((uint8_t)(color & 0x0F));
    uint32_t bg = fb_color((uint8_t)(color >> 4));

    unsigned int x0 = cx * FB_CELL_W;
    unsigned int y0 = cy * FB_CELL_H;

    for (unsigned int y = 0; y < FB_CELL_H; ++y)
        for (unsigned int x = 0; x < FB_CELL_W; ++x)
            fb_pixel(x0 + x, y0 + y, bg);

    for (unsigned int y = 0; y < 7; ++y)
        for (unsigned int x = 0; x < 5; ++x)
            if (g[y] & (1u << (4 - x)))
                fb_pixel(x0 + x, y0 + y, fg);
}

static void move_cursor(void)
{
    if (fb_mode) return;

    unsigned short position =
        (unsigned short)(row * VGA_WIDTH + column);
    __asm__ volatile ("outb %0, %1" :
        : "a"((uint8_t)0x0F), "Nd"((uint16_t)0x3D4));
    __asm__ volatile ("outb %0, %1" :
        : "a"((uint8_t)(position & 0xFF)), "Nd"((uint16_t)0x3D5));
    __asm__ volatile ("outb %0, %1" :
        : "a"((uint8_t)0x0E), "Nd"((uint16_t)0x3D4));
    __asm__ volatile ("outb %0, %1" :
        : "a"((uint8_t)(position >> 8)), "Nd"((uint16_t)0x3D5));
}

static void scroll(void)
{
    if (fb_mode) {
        unsigned int h = boot_info->framebuffer_height;
        unsigned int w = boot_info->framebuffer_width;
        for (unsigned int y = FB_CELL_H; y < h; ++y)
            for (unsigned int x = 0; x < w; ++x) {
                volatile uint8_t *fb =
                    (volatile uint8_t *)(uint64_t)boot_info->framebuffer;
                uint8_t *dst = (uint8_t *)((uint64_t)fb +
                    (y - FB_CELL_H) * boot_info->framebuffer_pitch + x * 4);
                volatile uint8_t *src = fb +
                    y * boot_info->framebuffer_pitch + x * 4;
                dst[0]=src[0]; dst[1]=src[1]; dst[2]=src[2]; dst[3]=src[3];
            }

        unsigned int cols = boot_info->framebuffer_width / FB_CELL_W;
        unsigned int rows = boot_info->framebuffer_height / FB_CELL_H;
        for (unsigned int x = 0; x < cols; ++x)
            fb_draw_char(x, rows - 1, ' ');
        row = rows - 1;
        return;
    }

    volatile uint16_t *vga = VGA_MEMORY;
    for (unsigned int y = 1; y < VGA_HEIGHT; ++y)
        for (unsigned int x = 0; x < VGA_WIDTH; ++x)
            vga[(y - 1) * VGA_WIDTH + x] = vga[y * VGA_WIDTH + x];

    for (unsigned int x = 0; x < VGA_WIDTH; ++x)
        vga[(VGA_HEIGHT - 1) * VGA_WIDTH + x] =
            ((uint16_t)color << 8) | ' ';

    row = VGA_HEIGHT - 1;
}

void terminal_clear(void)
{
    if (fb_mode) {
        fb_fill(fb_color((uint8_t)(color >> 4)));
        row = column = 0;
        return;
    }

    volatile uint16_t *vga = VGA_MEMORY;
    for (unsigned int i = 0; i < VGA_WIDTH * VGA_HEIGHT; ++i)
        vga[i] = ((uint16_t)color << 8) | ' ';
    row = column = 0;
    move_cursor();
}

void terminal_putchar(char c)
{
    if (fb_mode) {
        unsigned int cols = boot_info->framebuffer_width / FB_CELL_W;
        unsigned int rows = boot_info->framebuffer_height / FB_CELL_H;

        if (c == '\n') { column = 0; ++row; }
        else if (c == '\r') column = 0;
        else if (c == '\b') {
            if (column > 0) {
                --column;
                fb_draw_char(column, row, ' ');
            }
        } else {
            if (column >= cols) { column = 0; ++row; }
            if (row >= rows) scroll();
            fb_draw_char(column, row, c);
            ++column;
        }

        if (row >= rows) scroll();
        return;
    }

    volatile uint16_t *vga = VGA_MEMORY;

    if (c == '\n') {
        column = 0; ++row;
    } else if (c == '\r') {
        column = 0;
    } else if (c == '\b') {
        if (column > 0) {
            --column;
            vga[row * VGA_WIDTH + column] =
                ((uint16_t)color << 8) | ' ';
        }
    } else {
        if (column >= VGA_WIDTH) { column = 0; ++row; }
        if (row >= VGA_HEIGHT) scroll();
        vga[row * VGA_WIDTH + column] =
            ((uint16_t)color << 8) | (uint8_t)c;
        ++column;
    }

    if (row >= VGA_HEIGHT) scroll();
    move_cursor();
}

void terminal_puts(const char *s)
{
    while (*s) terminal_putchar(*s++);
}

void terminal_init(const struct zev_boot_info *info)
{
    boot_info = info;
    fb_mode = info && info->framebuffer &&
              info->framebuffer_width >= FB_CELL_W &&
              info->framebuffer_height >= FB_CELL_H &&
              info->framebuffer_pitch >= info->framebuffer_width * 4;
    terminal_clear();
}

void terminal_set_color(uint8_t new_color)
{
    color = new_color;
}

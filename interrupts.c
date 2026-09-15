/* ZevOS IDT, PIC, timer, and IRQ dispatch */

#include <stdint.h>

#define IDT_ENTRIES 256
#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1
#define PIC_EOI 0x20
#define PIT_COMMAND 0x43
#define PIT_CHANNEL0 0x40

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[IDT_ENTRIES];
static uint64_t ticks;

#define DECL_ISR(n) extern void isr##n(void)
#define DECL_IRQ(n) extern void irq##n(void)
DECL_ISR(0); DECL_ISR(1); DECL_ISR(2); DECL_ISR(3); DECL_ISR(4); DECL_ISR(5); DECL_ISR(6); DECL_ISR(7);
DECL_ISR(8); DECL_ISR(9); DECL_ISR(10); DECL_ISR(11); DECL_ISR(12); DECL_ISR(13); DECL_ISR(14); DECL_ISR(15);
DECL_ISR(16); DECL_ISR(17); DECL_ISR(18); DECL_ISR(19); DECL_ISR(20); DECL_ISR(21); DECL_ISR(22); DECL_ISR(23);
DECL_ISR(24); DECL_ISR(25); DECL_ISR(26); DECL_ISR(27); DECL_ISR(28); DECL_ISR(29); DECL_ISR(30); DECL_ISR(31);
DECL_IRQ(0); DECL_IRQ(1); DECL_IRQ(2); DECL_IRQ(3); DECL_IRQ(4); DECL_IRQ(5); DECL_IRQ(6); DECL_IRQ(7);
DECL_IRQ(8); DECL_IRQ(9); DECL_IRQ(10); DECL_IRQ(11); DECL_IRQ(12); DECL_IRQ(13); DECL_IRQ(14); DECL_IRQ(15);

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void idt_set_gate(unsigned int n, void (*handler)(void))
{
    uint64_t address = (uint64_t)handler;
    idt[n].offset_low = address & 0xFFFF;
    idt[n].selector = 0x08;
    idt[n].ist = 0;
    idt[n].type_attr = 0x8E;
    idt[n].offset_mid = (address >> 16) & 0xFFFF;
    idt[n].offset_high = (address >> 32) & 0xFFFFFFFF;
    idt[n].zero = 0;
}

static void pic_remap(void)
{
    uint8_t a1 = inb(PIC1_DATA);
    uint8_t a2 = inb(PIC2_DATA);

    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);
    outb(PIC1_DATA, 0x20);
    outb(PIC2_DATA, 0x28);
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    /* Start with timer + keyboard enabled; mask everything else. */
    outb(PIC1_DATA, (uint8_t)(a1 | 0xFC));
    outb(PIC2_DATA, (uint8_t)(a2 | 0xFF));
    outb(PIC1_DATA, inb(PIC1_DATA) & (uint8_t)~0x03);
}

static void pit_init(void)
{
    /* PIT channel 0, square-wave mode, ~100 Hz. */
    uint16_t divisor = 1193182 / 100;
    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, divisor >> 8);
}

void idt_init(void)
{
    for (unsigned int i = 0; i < IDT_ENTRIES; ++i) {
        idt[i].offset_low = 0;
        idt[i].selector = 0;
        idt[i].ist = 0;
        idt[i].type_attr = 0;
        idt[i].offset_mid = 0;
        idt[i].offset_high = 0;
        idt[i].zero = 0;
    }

    void (*exceptions[32])(void) = {
        isr0,isr1,isr2,isr3,isr4,isr5,isr6,isr7,isr8,isr9,isr10,isr11,isr12,isr13,isr14,isr15,
        isr16,isr17,isr18,isr19,isr20,isr21,isr22,isr23,isr24,isr25,isr26,isr27,isr28,isr29,isr30,isr31
    };
    void (*irqs[16])(void) = {
        irq0,irq1,irq2,irq3,irq4,irq5,irq6,irq7,irq8,irq9,irq10,irq11,irq12,irq13,irq14,irq15
    };

    for (unsigned int i = 0; i < 32; ++i) idt_set_gate(i, exceptions[i]);
    for (unsigned int i = 0; i < 16; ++i) idt_set_gate(32 + i, irqs[i]);

    struct idt_ptr idtr = { sizeof(idt) - 1, (uint64_t)idt };
    __asm__ volatile ("lidt %0" : : "m"(idtr));

    pic_remap();
    pit_init();
}

void exception_handler(uint64_t *stack)
{
    uint64_t vector = stack[15];
    terminal_puts("\nKERNEL EXCEPTION: ");
    terminal_puts("vector ");
    terminal_putchar('0' + (char)(vector % 10));
    terminal_puts("\nSystem halted.\n");
    __asm__ volatile ("cli");
    for (;;) __asm__ volatile ("hlt");
}

extern void keyboard_irq(void);

void irq_handler(uint64_t *stack)
{
    uint64_t vector = stack[15];
    if (vector == 32)
        ++ticks;
    else if (vector == 33)
        keyboard_irq();

    if (vector >= 40)
        outb(PIC2_COMMAND, PIC_EOI);
    if (vector >= 32)
        outb(PIC1_COMMAND, PIC_EOI);
}

uint64_t timer_ticks(void)
{
    return ticks;
}

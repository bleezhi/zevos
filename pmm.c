/* ZevOS physical page-frame allocator.
 * Uses the Multiboot2 memory map and 4 KiB pages.
 */

#include <stdint.h>

#define PAGE_SIZE 4096ULL
#define MAX_MEMORY (1ULL << 30)
#define PAGE_COUNT (MAX_MEMORY / PAGE_SIZE)
#define BITMAP_SIZE (PAGE_COUNT / 8)

static uint8_t page_bitmap[BITMAP_SIZE];
static uint64_t total_pages;
static uint64_t free_pages;

extern char kernel_start[];
extern char kernel_end[];

static void mark_used(uint64_t page)
{
    if (page >= PAGE_COUNT) return;
    page_bitmap[page >> 3] |= (uint8_t)(1u << (page & 7));
}

static void mark_free(uint64_t page)
{
    if (page >= PAGE_COUNT) return;
    page_bitmap[page >> 3] &= (uint8_t)~(1u << (page & 7));
}

static int is_used(uint64_t page)
{
    return (page_bitmap[page >> 3] >> (page & 7)) & 1;
}

void pmm_init(uint32_t multiboot_info)
{
    for (unsigned int i = 0; i < BITMAP_SIZE; ++i)
        page_bitmap[i] = 0xFF;

    total_pages = PAGE_COUNT;
    free_pages = 0;

    uint8_t *base = (uint8_t *)(uint64_t)multiboot_info;
    uint32_t total_size = *(uint32_t *)base;
    uint32_t offset = 8;

    while (offset + 8 <= total_size) {
        uint32_t type = *(uint32_t *)(base + offset);
        uint32_t size = *(uint32_t *)(base + offset + 4);
        if (size < 8 || offset + size > total_size)
            break;

        if (type == 6) {
            uint64_t entry_size = *(uint32_t *)(base + offset + 8);
            if (entry_size >= 24) {
                uint8_t *entry = base + offset + 16;
                while (entry + entry_size <= base + offset + size) {
                    uint64_t addr = *(uint64_t *)entry;
                    uint64_t len = *(uint64_t *)(entry + 8);
                    uint32_t kind = *(uint32_t *)(entry + 16);
                    if (kind == 1 && addr < MAX_MEMORY) {
                        uint64_t end = addr + len;
                        if (end > MAX_MEMORY) end = MAX_MEMORY;
                        uint64_t first = (addr + PAGE_SIZE - 1) / PAGE_SIZE;
                        uint64_t last = end / PAGE_SIZE;
                        for (uint64_t p = first; p < last; ++p) mark_free(p);
                    }
                    entry += entry_size;
                }
            }
        }

        offset = (offset + size + 7) & ~7u;
    }

    /* Never hand out pages belonging to the kernel itself. */
    uint64_t ks = (uint64_t)kernel_start / PAGE_SIZE;
    uint64_t ke = ((uint64_t)kernel_end + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint64_t p = ks; p < ke; ++p) mark_used(p);

    /* Reserve the Multiboot information structure. */
    uint64_t ms = multiboot_info / PAGE_SIZE;
    uint64_t me = ((uint64_t)multiboot_info + total_size + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint64_t p = ms; p < me; ++p) mark_used(p);

    for (uint64_t p = 0; p < PAGE_COUNT; ++p)
        if (!is_used(p)) ++free_pages;
}

void *page_alloc(void)
{
    for (uint64_t p = 1; p < PAGE_COUNT; ++p) {
        if (!is_used(p)) {
            mark_used(p);
            --free_pages;
            return (void *)(p * PAGE_SIZE);
        }
    }
    return 0;
}

void page_free(void *address)
{
    uint64_t addr = (uint64_t)address;
    if ((addr & (PAGE_SIZE - 1)) != 0 || addr >= MAX_MEMORY)
        return;
    uint64_t page = addr / PAGE_SIZE;
    if (is_used(page)) {
        mark_free(page);
        ++free_pages;
    }
}

uint64_t pmm_free_pages(void)
{
    return free_pages;
}

uint64_t pmm_total_pages(void)
{
    return total_pages;
}

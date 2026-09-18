/* ZevOS physical page-frame allocator using ZevBoot's memory map. */
#include <stdint.h>
#include "zevboot.h"

#define PAGE_SIZE 4096ULL
#define MAX_MEMORY (1ULL << 32)
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

void pmm_init(const struct zev_boot_info *boot_info)
{
    for (unsigned int i = 0; i < BITMAP_SIZE; ++i)
        page_bitmap[i] = 0xFF;

    total_pages = PAGE_COUNT;
    free_pages = 0;

    if (!boot_info ||
        boot_info->magic != ZEV_BOOT_MAGIC ||
        !boot_info->memory_map ||
        !boot_info->memory_map_entry_size)
        return;

    uint8_t *base = (uint8_t *)(uint64_t)boot_info->memory_map;

    for (uint64_t i = 0; i < boot_info->memory_map_entries; ++i) {
        struct zev_memory_map_entry *entry =
            (struct zev_memory_map_entry *)(base +
                i * boot_info->memory_map_entry_size);

        if (entry->type != 1 || entry->length == 0)
            continue;

        uint64_t addr = entry->base;
        uint64_t end = addr + entry->length;
        if (end < addr) end = MAX_MEMORY;
        if (addr >= MAX_MEMORY) continue;
        if (end > MAX_MEMORY) end = MAX_MEMORY;

        uint64_t first = (addr + PAGE_SIZE - 1) / PAGE_SIZE;
        uint64_t last = end / PAGE_SIZE;
        for (uint64_t p = first; p < last; ++p)
            mark_free(p);
    }

    /* Never allocate pages occupied by the kernel. */
    uint64_t ks = (uint64_t)kernel_start / PAGE_SIZE;
    uint64_t ke = ((uint64_t)kernel_end + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint64_t p = ks; p < ke; ++p)
        mark_used(p);

    /* Keep the ZevBootInfo and its memory map alive. */
    uint64_t bs = (uint64_t)boot_info / PAGE_SIZE;
    uint64_t be = bs + 1;
    for (uint64_t p = bs; p < be; ++p)
        mark_used(p);

    uint64_t ms = boot_info->memory_map / PAGE_SIZE;
    uint64_t map_bytes =
        boot_info->memory_map_entries * boot_info->memory_map_entry_size;
    uint64_t me = (boot_info->memory_map + map_bytes + PAGE_SIZE - 1) /
                  PAGE_SIZE;
    for (uint64_t p = ms; p < me; ++p)
        mark_used(p);

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

uint64_t pmm_free_pages(void) { return free_pages; }
uint64_t pmm_total_pages(void) { return total_pages; }

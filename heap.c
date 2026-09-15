/* ZevOS early kernel heap.
 * This is intentionally simple: allocations come from whole physical pages.
 * A proper virtual heap can replace this once page tables are expanded.
 */

#include <stdint.h>

void *page_alloc(void);

#define HEAP_PAGE_SIZE 4096
#define HEAP_BLOCKS 1024

struct heap_page {
    uint8_t used[HEAP_PAGE_SIZE];
};

static uint8_t *pages[HEAP_BLOCKS];
static unsigned int page_count;
static unsigned int offset;

void *kmalloc(uint64_t size)
{
    if (size == 0 || size > HEAP_PAGE_SIZE)
        return 0;

    if (page_count == 0 || offset + size > HEAP_PAGE_SIZE) {
        if (page_count >= HEAP_BLOCKS)
            return 0;
        pages[page_count] = (uint8_t *)page_alloc();
        if (!pages[page_count])
            return 0;
        ++page_count;
        offset = 0;
    }

    void *result = pages[page_count - 1] + offset;
    offset += (unsigned int)size;
    return result;
}

void kfree(void *ptr)
{
    /* Page ownership will be handled by the real heap allocator later. */
    (void)ptr;
}

/* ZevOS virtual-memory foundation.
 * Builds a fresh identity-mapped address space for the first 1 GiB.
 * User mappings are intentionally not enabled yet; this is the safe
 * foundation for per-process CR3 values and later ring-3 execution.
 */

#include <stdint.h>

#define PAGE_SIZE 4096ULL
#define PAGE_PRESENT 1ULL
#define PAGE_WRITE 2ULL
#define PAGE_USER 4ULL
#define PAGE_HUGE 128ULL
#define PAGE_TABLE_ENTRIES 512

extern void *page_alloc(void);

static uint64_t *kernel_pml4;

static void zero_page(uint64_t *page)
{
    for (unsigned int i = 0; i < PAGE_TABLE_ENTRIES; ++i)
        page[i] = 0;
}

static uint64_t *new_table(void)
{
    uint64_t *page = (uint64_t *)page_alloc();
    if (!page)
        return 0;
    zero_page(page);
    return page;
}

static uint64_t *build_identity_space(void)
{
    uint64_t *pml4 = new_table();
    uint64_t *pdpt = new_table();
    uint64_t *pd = new_table();

    if (!pml4 || !pdpt || !pd)
        return 0;

    pml4[0] = (uint64_t)pdpt | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    pdpt[0] = (uint64_t)pd | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;

    for (unsigned int i = 0; i < PAGE_TABLE_ENTRIES; ++i)
        pd[i] = ((uint64_t)i << 21) | PAGE_PRESENT | PAGE_WRITE | PAGE_HUGE;

    return pml4;
}

void vmm_init(void)
{
    kernel_pml4 = build_identity_space();
}

uint64_t vmm_kernel_cr3(void)
{
    return (uint64_t)kernel_pml4;
}

uint64_t vmm_create_address_space(void)
{
    uint64_t *pml4 = build_identity_space();
    return (uint64_t)pml4;
}

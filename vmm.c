/* ZevOS virtual-memory subsystem. */

#include <stdint.h>

#define PAGE_SIZE 4096ULL
#define PAGE_PRESENT 1ULL
#define PAGE_WRITE 2ULL
#define PAGE_USER 4ULL
#define PAGE_HUGE 128ULL
#define PAGE_TABLE_ENTRIES 512

#define USER_MIN 0x400000ULL
#define USER_MAX 0x800000ULL

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

    pml4[0] = (uint64_t)pdpt | PAGE_PRESENT | PAGE_WRITE;
    pdpt[0] = (uint64_t)pd | PAGE_PRESENT | PAGE_WRITE;

    for (unsigned int i = 0; i < PAGE_TABLE_ENTRIES; ++i)
        pd[i] = ((uint64_t)i << 21) | PAGE_PRESENT | PAGE_WRITE | PAGE_HUGE;

    return pml4;
}

void vmm_init(void)
{
    kernel_pml4 = build_identity_space();

    if (kernel_pml4)
        __asm__ volatile ("mov %0, %%cr3" :: "r"((uint64_t)kernel_pml4) : "memory");
}

uint64_t vmm_kernel_cr3(void)
{
    return (uint64_t)kernel_pml4;
}

uint64_t vmm_create_address_space(void)
{
    return (uint64_t)build_identity_space();
}

/* Split a 2 MiB identity mapping into 512 normal 4 KiB mappings. */
static uint64_t *split_huge_page(uint64_t *pd, unsigned int pd_i)
{
    uint64_t entry = pd[pd_i];
    if (!(entry & PAGE_PRESENT) || !(entry & PAGE_HUGE))
        return 0;

    uint64_t *pt = new_table();
    if (!pt)
        return 0;

    uint64_t base = entry & ~0x1FFFFFULL;
    uint64_t flags = entry & 0xFFFULL;
    flags &= ~PAGE_HUGE;

    for (unsigned int i = 0; i < PAGE_TABLE_ENTRIES; ++i)
        pt[i] = (base + ((uint64_t)i * PAGE_SIZE)) | flags;

    pd[pd_i] = (uint64_t)pt | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    return pt;
}

/* Map one user virtual page to an already allocated physical page. */
int vmm_map_user_page(uint64_t cr3, uint64_t virtual_address, uint64_t physical_address)
{
    if (!cr3 || (virtual_address & (PAGE_SIZE - 1)) != 0 ||
        (physical_address & (PAGE_SIZE - 1)) != 0 ||
        virtual_address < USER_MIN || virtual_address >= USER_MAX)
        return -1;

    uint64_t *pml4 = (uint64_t *)cr3;
    unsigned int pml4_i = (unsigned int)((virtual_address >> 39) & 0x1FF);
    unsigned int pdpt_i = (unsigned int)((virtual_address >> 30) & 0x1FF);
    unsigned int pd_i   = (unsigned int)((virtual_address >> 21) & 0x1FF);
    unsigned int pt_i   = (unsigned int)((virtual_address >> 12) & 0x1FF);

    uint64_t *pdpt;
    uint64_t *pd;
    uint64_t *pt;

    if (!(pml4[pml4_i] & PAGE_PRESENT)) {
        pdpt = new_table();
        if (!pdpt) return -1;
        pml4[pml4_i] = (uint64_t)pdpt | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    } else {
        pdpt = (uint64_t *)(pml4[pml4_i] & ~0xFFFULL);
        pml4[pml4_i] |= PAGE_USER;
    }

    if (!(pdpt[pdpt_i] & PAGE_PRESENT)) {
        pd = new_table();
        if (!pd) return -1;
        pdpt[pdpt_i] = (uint64_t)pd | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    } else {
        if (pdpt[pdpt_i] & PAGE_HUGE) return -1;
        pd = (uint64_t *)(pdpt[pdpt_i] & ~0xFFFULL);
        pdpt[pdpt_i] |= PAGE_USER;
    }

    if (!(pd[pd_i] & PAGE_PRESENT)) {
        pt = new_table();
        if (!pt) return -1;
        pd[pd_i] = (uint64_t)pt | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    } else if (pd[pd_i] & PAGE_HUGE) {
        pt = split_huge_page(pd, pd_i);
        if (!pt) return -1;
    } else {
        pt = (uint64_t *)(pd[pd_i] & ~0xFFFULL);
        pd[pd_i] |= PAGE_USER;
    }

    /* A split identity mapping already contains a supervisor-only PTE at
     * this address. Replace that mapping with the requested user page.
     * Refuse to silently replace an existing user mapping. */
    if (pt[pt_i] & PAGE_PRESENT) {
        if (pt[pt_i] & PAGE_USER)
            return -1;
    }

    pt[pt_i] = physical_address | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    return 0;
}

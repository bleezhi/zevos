/* ZevOS minimal ELF64 loader for the first userspace program. */
#include <stdint.h>

#define PAGE_SIZE 4096ULL
#define PAGE_PRESENT 1ULL
#define PT_LOAD 1U
#define PF_X 1U
#define PF_W 2U
#define PF_R 4U

struct elf64_ehdr {
    unsigned char ident[16];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint64_t entry;
    uint64_t phoff;
    uint64_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
};

struct elf64_phdr {
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
    uint64_t vaddr;
    uint64_t paddr;
    uint64_t filesz;
    uint64_t memsz;
    uint64_t align;
};

extern void *page_alloc(void);
extern void page_free(void *address);
extern int vmm_map_user_page(uint64_t cr3, uint64_t virtual_address, uint64_t physical_address);

static int range_ok(uint64_t offset, uint64_t size, uint64_t total)
{
    return offset <= total && size <= total - offset;
}

/* Loads a deliberately small ELF image into one or more user pages.
 * Current bootstrap images are limited to a single 4 KiB PT_LOAD segment. */
int elf_load_image(uint64_t cr3, const void *image, uint64_t image_size,
                   uint64_t *entry_out, uint64_t *first_page_out)
{
    const struct elf64_ehdr *eh = (const struct elf64_ehdr *)image;
    uint64_t first_page = 0;

    if (!cr3 || !image || !entry_out || !first_page_out || image_size < sizeof(*eh))
        return -1;
    if (eh->ident[0] != 0x7F || eh->ident[1] != 'E' ||
        eh->ident[2] != 'L' || eh->ident[3] != 'F' || eh->ident[4] != 2)
        return -2;
    if (eh->machine != 0x3E || eh->type != 2 || eh->version != 1)
        return -3;
    if (eh->phentsize != sizeof(struct elf64_phdr) || eh->phnum == 0 ||
        !range_ok(eh->phoff, (uint64_t)eh->phnum * eh->phentsize, image_size))
        return -4;

    for (uint16_t i = 0; i < eh->phnum; ++i) {
        const struct elf64_phdr *ph =
            (const struct elf64_phdr *)((const uint8_t *)image + eh->phoff +
                                        (uint64_t)i * eh->phentsize);
        if (ph->type != PT_LOAD)
            continue;
        if (!range_ok(ph->offset, ph->filesz, image_size) ||
            ph->memsz < ph->filesz || ph->memsz == 0 ||
            ph->vaddr < 0x400000ULL || ph->vaddr >= 0x800000ULL ||
            ph->memsz > PAGE_SIZE || (ph->vaddr & (PAGE_SIZE - 1)) != 0)
            return -5;

        void *page = page_alloc();
        if (!page)
            return -6;

        if (vmm_map_user_page(cr3, ph->vaddr, (uint64_t)page) != 0) {
            page_free(page);
            return -7;
        }

        uint8_t *dst = (uint8_t *)page;
        const uint8_t *src = (const uint8_t *)image + ph->offset;
        for (uint64_t n = 0; n < ph->filesz; ++n)
            dst[n] = src[n];
        for (uint64_t n = ph->filesz; n < ph->memsz; ++n)
            dst[n] = 0;

        if (!first_page)
            first_page = (uint64_t)page;
    }

    if (!first_page || eh->entry < 0x400000ULL || eh->entry >= 0x800000ULL)
        return -8;

    *entry_out = eh->entry;
    *first_page_out = first_page;
    return 0;
}

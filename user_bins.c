/* Built-in ELF64 /bin programs for the initial ramfs. */
#include <stdint.h>
#include "vfs.h"

struct elf64_ehdr {
    unsigned char ident[16]; uint16_t type; uint16_t machine; uint32_t version;
    uint64_t entry; uint64_t phoff; uint64_t shoff; uint32_t flags;
    uint16_t ehsize; uint16_t phentsize; uint16_t phnum; uint16_t shentsize;
    uint16_t shnum; uint16_t shstrndx;
};
struct elf64_phdr {
    uint32_t type; uint32_t flags; uint64_t offset; uint64_t vaddr;
    uint64_t paddr; uint64_t filesz; uint64_t memsz; uint64_t align;
};

static uint64_t u32(uint32_t v) { return v; }

/* Make a tiny ET_EXEC that exercises the real userspace syscall path. */
static uint64_t make_program(uint8_t *image, const char *message)
{
    uint64_t msg_len = 0;
    while (message[msg_len]) ++msg_len;

    for (unsigned int i = 0; i < 512; ++i) image[i] = 0;

    struct elf64_ehdr *eh = (struct elf64_ehdr *)image;
    eh->ident[0] = 0x7f; eh->ident[1] = 'E'; eh->ident[2] = 'L'; eh->ident[3] = 'F';
    eh->ident[4] = 2; eh->ident[5] = 1; eh->ident[6] = 1;
    eh->type = 2; eh->machine = 0x3e; eh->version = 1;
    eh->entry = 0x400100; eh->phoff = 64; eh->ehsize = 64;
    eh->phentsize = 56; eh->phnum = 1;

    struct elf64_phdr *ph = (struct elf64_phdr *)(image + 64);
    ph->type = 1; ph->flags = 5; ph->offset = 0x100; ph->vaddr = 0x400000;
    ph->filesz = 0x3a + msg_len; ph->memsz = ph->filesz; ph->align = 0x1000;

    uint8_t *code = image + 0x100;
    unsigned int p = 0;
    /* mov rax, SYS_WRITE */
    code[p++] = 0x48; code[p++] = 0xc7; code[p++] = 0xc0;
    code[p++] = 1; code[p++] = 0; code[p++] = 0; code[p++] = 0;
    /* mov rdi, stdout */
    code[p++] = 0x48; code[p++] = 0xc7; code[p++] = 0xc7;
    code[p++] = 1; code[p++] = 0; code[p++] = 0; code[p++] = 0;
    /* mov rsi, message virtual address */
    code[p++] = 0x48; code[p++] = 0xbe;
    uint64_t addr = 0x400100 + 0x3a;
    for (unsigned int i = 0; i < 8; ++i) code[p++] = (uint8_t)(addr >> (i * 8));
    /* mov rdx, message length */
    code[p++] = 0x48; code[p++] = 0xc7; code[p++] = 0xc2;
    code[p++] = (uint8_t)msg_len; code[p++] = 0; code[p++] = 0; code[p++] = 0;
    /* int 0x80 */
    code[p++] = 0xcd; code[p++] = 0x80;
    /* mov rax, SYS_EXIT */
    code[p++] = 0x48; code[p++] = 0xc7; code[p++] = 0xc0;
    code[p++] = 3; code[p++] = 0; code[p++] = 0; code[p++] = 0;
    /* xor rdi, rdi; int 0x80 */
    code[p++] = 0x48; code[p++] = 0x31; code[p++] = 0xff;
    code[p++] = 0xcd; code[p++] = 0x80;

    for (uint64_t i = 0; i < msg_len; ++i) image[0x100 + 0x3a + i] = (uint8_t)message[i];
    (void)u32;
    return 0x100 + 0x3a + msg_len;
}

void vfs_install_binaries(void)
{
    uint8_t image[512];
    make_program(image, "echo: ZevOS\n");
    vfs_touch("/bin/echo"); vfs_write_binary("/bin/echo", image, 0x100 + 0x3a + 12);
    make_program(image, "cat: /etc/motd\n");
    vfs_touch("/bin/cat"); vfs_write_binary("/bin/cat", image, 0x100 + 0x3a + 15);
    make_program(image, "ls: /bin /dev /etc /home /tmp /usr /var\n");
    vfs_touch("/bin/ls"); vfs_write_binary("/bin/ls", image, 0x100 + 0x3a + 40);
    make_program(image, "pwd: /\n");
    vfs_touch("/bin/pwd"); vfs_write_binary("/bin/pwd", image, 0x100 + 0x3a + 7);
    make_program(image, "true\n");
    vfs_touch("/bin/true"); vfs_write_binary("/bin/true", image, 0x100 + 0x3a + 5);
    make_program(image, "false\n");
    vfs_touch("/bin/false"); vfs_write_binary("/bin/false", image, 0x100 + 0x3a + 6);
    make_program(image, "zinit: userspace PID 1\n");
    vfs_touch("/bin/zinit"); vfs_write_binary("/bin/zinit", image, 0x100 + 0x3a + 23);
    make_program(image, "dsplayed: display server\n");
    vfs_touch("/bin/dsplayed"); vfs_write_binary("/bin/dsplayed", image, 0x100 + 0x3a + 26);
}

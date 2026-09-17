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

static unsigned int str_len(const char *s)
{
    unsigned int n = 0;
    while (s[n]) ++n;
    return n;
}

/* Build a tiny ET_EXEC that uses the real SYS_WRITE/SYS_EXIT ABI.
 * If stay_alive is set, the process remains in ring 3 after writing. */
static uint64_t make_program(uint8_t *image, const char *message, int stay_alive)
{
    const unsigned int code_offset = 0x100;
    const unsigned int message_offset_base = 0x100;
    unsigned int message_length = str_len(message);
    unsigned int p = 0;

    for (unsigned int i = 0; i < 512; ++i) image[i] = 0;

    struct elf64_ehdr *eh = (struct elf64_ehdr *)image;
    eh->ident[0] = 0x7f; eh->ident[1] = 'E'; eh->ident[2] = 'L'; eh->ident[3] = 'F';
    eh->ident[4] = 2; eh->ident[5] = 1; eh->ident[6] = 1;
    eh->type = 2; eh->machine = 0x3e; eh->version = 1;
    eh->entry = 0x400100; eh->phoff = 64; eh->ehsize = 64;
    eh->phentsize = 56; eh->phnum = 1;

    uint8_t *code = image + code_offset;
    /* mov rax, SYS_WRITE */
    code[p++] = 0x48; code[p++] = 0xc7; code[p++] = 0xc0;
    code[p++] = 1; code[p++] = 0; code[p++] = 0; code[p++] = 0;
    /* mov rdi, stdout */
    code[p++] = 0x48; code[p++] = 0xc7; code[p++] = 0xc7;
    code[p++] = 1; code[p++] = 0; code[p++] = 0; code[p++] = 0;
    /* mov rsi, message virtual address (filled after code length is known) */
    code[p++] = 0x48; code[p++] = 0xbe;
    unsigned int address_patch = p;
    for (unsigned int i = 0; i < 8; ++i) code[p++] = 0;
    /* mov rdx, message length */
    code[p++] = 0x48; code[p++] = 0xc7; code[p++] = 0xc2;
    code[p++] = (uint8_t)message_length; code[p++] = 0; code[p++] = 0; code[p++] = 0;
    /* int 0x80 */
    code[p++] = 0xcd; code[p++] = 0x80;

    if (stay_alive) {
        /* jmp $ -- keep the display server alive in ring 3 for stage 0. */
        code[p++] = 0xeb; code[p++] = 0xfe;
    } else {
        /* mov rax, SYS_EXIT; xor rdi,rdi; int 0x80 */
        code[p++] = 0x48; code[p++] = 0xc7; code[p++] = 0xc0;
        code[p++] = 3; code[p++] = 0; code[p++] = 0; code[p++] = 0;
        code[p++] = 0x48; code[p++] = 0x31; code[p++] = 0xff;
        code[p++] = 0xcd; code[p++] = 0x80;
    }

    uint64_t message_address = 0x400000ULL + code_offset + p;
    for (unsigned int i = 0; i < 8; ++i)
        code[address_patch + i] = (uint8_t)(message_address >> (i * 8));

    for (unsigned int i = 0; i < message_length; ++i)
        image[message_offset_base + p + i] = (uint8_t)message[i];

    struct elf64_phdr *ph = (struct elf64_phdr *)(image + 64);
    ph->type = 1; ph->flags = 5; ph->offset = code_offset; ph->vaddr = 0x400000;
    ph->filesz = p + message_length; ph->memsz = ph->filesz; ph->align = 0x1000;

    return code_offset + p + message_length;
}

static void install_program(const char *path, const char *message, int stay_alive)
{
    uint8_t image[512];
    uint64_t size = make_program(image, message, stay_alive);
    vfs_touch(path);
    vfs_write_binary(path, image, size);
}

void vfs_install_binaries(void)
{
    install_program("/bin/echo", "echo: ZevOS\n", 0);
    install_program("/bin/cat", "cat: /etc/motd\n", 0);
    install_program("/bin/ls", "ls: /bin /dev /etc /home /tmp /usr /var\n", 0);
    install_program("/bin/pwd", "pwd: /\n", 0);
    install_program("/bin/true", "true\n", 0);
    install_program("/bin/false", "false\n", 0);
    install_program("/bin/zinit", "zinit: userspace PID 1\n", 0);
    install_program("/bin/dsplayed", "dsplayed: display server\n", 1);
}

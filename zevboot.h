#ifndef ZEVBOOT_H
#define ZEVBOOT_H

#include <stdint.h>

#define ZEV_BOOT_MAGIC 0x5A4556424F4F544FULL
#define ZEV_BOOT_VERSION 1ULL

struct zev_memory_map_entry {
    uint64_t base;
    uint64_t length;
    uint32_t type;       /* 1 = usable RAM, other values = reserved/firmware */
    uint32_t reserved;
};

struct zev_boot_info {
    uint64_t magic;
    uint64_t version;

    uint64_t memory_map;
    uint64_t memory_map_entries;
    uint64_t memory_map_entry_size;

    uint64_t framebuffer;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_format;

    uint64_t kernel_base;
    uint64_t kernel_end;

    uint64_t boot_drive;
    uint64_t flags;
};

#endif

#ifndef ZEVOS_ZEVFS_H
#define ZEVOS_ZEVFS_H
#include <stdint.h>
int zevfs_mount(void);
int zevfs_format(void);
int zevfs_write_file(const char *path,const void *data,uint32_t size);
int zevfs_read_file(const char *path,void *data,uint32_t capacity);
int zevfs_ready(void);
#endif

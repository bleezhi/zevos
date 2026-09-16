#ifndef ZEVOS_VFS_H
#define ZEVOS_VFS_H

#include <stdint.h>

#define VFS_FILE 1
#define VFS_DIR 2

void vfs_init(void);
int vfs_exists(const char *path);
int vfs_is_dir(const char *path);
int vfs_mkdir(const char *path);
int vfs_touch(const char *path);
int vfs_remove(const char *path);
int vfs_read(const char *path, char *buffer, uint64_t size);
int vfs_write(const char *path, const char *data, uint64_t size);
int vfs_copy(const char *source, const char *destination);
int vfs_list(const char *path, void (*emit)(const char *name, int type));

#endif

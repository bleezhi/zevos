/* ZevOS VFS + tiny in-memory ramfs. */
#include <stdint.h>
#include "vfs.h"

#define VFS_MAX_NODES 128
#define VFS_NAME_MAX 24
#define VFS_DATA_MAX 512

struct vfs_node {
    uint8_t used;
    uint8_t type;
    int16_t parent;
    char name[VFS_NAME_MAX];
    char data[VFS_DATA_MAX];
    uint32_t size;
};

static struct vfs_node nodes[VFS_MAX_NODES];

static int str_eq(const char *a, const char *b)
{
    while (*a && *b && *a == *b) { ++a; ++b; }
    return *a == 0 && *b == 0;
}

static unsigned int str_len(const char *s)
{
    unsigned int n = 0;
    while (s[n]) ++n;
    return n;
}

static void str_copy(char *dst, const char *src, unsigned int max)
{
    unsigned int i = 0;
    while (src[i] && i + 1 < max) { dst[i] = src[i]; ++i; }
    dst[i] = 0;
}

static int child(int parent, const char *name)
{
    for (int i = 0; i < VFS_MAX_NODES; ++i)
        if (nodes[i].used && nodes[i].parent == parent && str_eq(nodes[i].name, name))
            return i;
    return -1;
}

/* Paths are absolute here; the shell converts relative paths first. */
static int lookup(const char *path)
{
    if (!path || path[0] != '/') return -1;
    if (path[1] == 0) return 0;

    int current = 0;
    unsigned int i = 1;
    char part[VFS_NAME_MAX];
    while (path[i]) {
        unsigned int n = 0;
        while (path[i] == '/') ++i;
        while (path[i] && path[i] != '/' && n + 1 < VFS_NAME_MAX)
            part[n++] = path[i++];
        part[n] = 0;
        if (!n || str_eq(part, ".")) continue;
        if (str_eq(part, "..")) {
            if (nodes[current].parent >= 0) current = nodes[current].parent;
            continue;
        }
        current = child(current, part);
        if (current < 0) return -1;
    }
    return current;
}

static int split_parent(const char *path, char *parent, char *name)
{
    unsigned int len = str_len(path);
    if (!path || path[0] != '/' || len == 0 || len >= 256) return -1;

    char tmp[256];
    for (unsigned int i = 0; i <= len; ++i) tmp[i] = path[i];
    while (len > 1 && tmp[len - 1] == '/') tmp[--len] = 0;

    int slash = -1;
    for (unsigned int i = 1; i < len; ++i)
        if (tmp[i] == '/') slash = (int)i;

    if (slash < 0) {
        str_copy(parent, "/", 256);
        str_copy(name, tmp + 1, VFS_NAME_MAX);
    } else {
        tmp[slash] = 0;
        str_copy(parent, tmp, 256);
        str_copy(name, tmp + slash + 1, VFS_NAME_MAX);
    }
    return name[0] ? 0 : -1;
}

static int create_node(const char *path, int type)
{
    char parent_path[256], name[VFS_NAME_MAX];
    if (split_parent(path, parent_path, name) != 0 || str_eq(name, ".") || str_eq(name, "..")) return -1;
    int parent = lookup(parent_path);
    if (parent < 0 || nodes[parent].type != VFS_DIR || child(parent, name) >= 0) return -1;

    int slot = -1;
    for (int i = 1; i < VFS_MAX_NODES; ++i)
        if (!nodes[i].used) { slot = i; break; }
    if (slot < 0) return -1;

    nodes[slot].used = 1;
    nodes[slot].type = (uint8_t)type;
    nodes[slot].parent = (int16_t)parent;
    nodes[slot].size = 0;
    nodes[slot].name[0] = 0;
    str_copy(nodes[slot].name, name, VFS_NAME_MAX);
    return slot;
}

void vfs_init(void)
{
    for (int i = 0; i < VFS_MAX_NODES; ++i) nodes[i].used = 0;
    nodes[0].used = 1;
    nodes[0].type = VFS_DIR;
    nodes[0].parent = 0;
    nodes[0].name[0] = 0;

    vfs_mkdir("/bin"); vfs_mkdir("/dev"); vfs_mkdir("/etc");
    vfs_mkdir("/home"); vfs_mkdir("/tmp"); vfs_mkdir("/usr"); vfs_mkdir("/var");
    vfs_mkdir("/usr/bin"); vfs_mkdir("/usr/lib");
    vfs_touch("/etc/hostname");
    vfs_write("/etc/hostname", "ZevOS\n", 6);
    vfs_touch("/etc/motd");
    vfs_write("/etc/motd", "Welcome to ZevOS.\n", 19);
}

int vfs_exists(const char *path) { return lookup(path) >= 0; }
int vfs_is_dir(const char *path)
{
    int n = lookup(path);
    return n >= 0 && nodes[n].type == VFS_DIR;
}

int vfs_mkdir(const char *path) { return create_node(path, VFS_DIR) >= 0 ? 0 : -1; }
int vfs_touch(const char *path) { return create_node(path, VFS_FILE) >= 0 ? 0 : -1; }

int vfs_remove(const char *path)
{
    int n = lookup(path);
    if (n <= 0) return -1;
    if (nodes[n].type == VFS_DIR)
        for (int i = 1; i < VFS_MAX_NODES; ++i)
            if (nodes[i].used && nodes[i].parent == n) return -2;
    nodes[n].used = 0;
    return 0;
}

int vfs_read(const char *path, char *buffer, uint64_t size)
{
    int n = lookup(path);
    if (n < 0 || nodes[n].type != VFS_FILE || !buffer || size == 0) return -1;
    uint64_t count = nodes[n].size < size - 1 ? nodes[n].size : size - 1;
    for (uint64_t i = 0; i < count; ++i) buffer[i] = nodes[n].data[i];
    buffer[count] = 0;
    return (int)count;
}

int vfs_write(const char *path, const char *data, uint64_t size)
{
    int n = lookup(path);
    if (n < 0 || nodes[n].type != VFS_FILE || !data || size >= VFS_DATA_MAX) return -1;
    for (uint64_t i = 0; i < size; ++i) nodes[n].data[i] = data[i];
    nodes[n].data[size] = 0;
    nodes[n].size = (uint32_t)size;
    return (int)size;
}

int vfs_copy(const char *source, const char *destination)
{
    int s = lookup(source);
    if (s < 0 || nodes[s].type != VFS_FILE) return -1;
    int d = lookup(destination);
    if (d < 0) {
        if (vfs_touch(destination) != 0) return -1;
        d = lookup(destination);
    }
    if (d < 0 || nodes[d].type != VFS_FILE) return -1;
    return vfs_write(destination, nodes[s].data, nodes[s].size) < 0 ? -1 : 0;
}

int vfs_list(const char *path, void (*emit)(const char *name, int type))
{
    int p = lookup(path);
    if (p < 0 || nodes[p].type != VFS_DIR || !emit) return -1;
    for (int i = 1; i < VFS_MAX_NODES; ++i)
        if (nodes[i].used && nodes[i].parent == p)
            emit(nodes[i].name, nodes[i].type);
    return 0;
}

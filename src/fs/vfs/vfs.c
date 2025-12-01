#include "fs/vfs.h"
#include "drivers/block/device.h"
#include "fs/filesystem.h"
#include "lib/stdlib.h"
#include "memory/kmalloc.h"

#include <ferrite/types.h>
#include <lib/string.h>

extern struct super_operations ext2_sops;
vfs_inode_t* root_inode = NULL;

/**
 * Creates initial directory structure: /sys, /var, /dev, /proc
 *
 * These directories persist on disk as mount points.
 * /var contains persistent files. /proc, /sys, and /dev will hold
 * RAM-based contents when virtual filesystems are mounted.
 *
 * NOTE:
 * /sys, /dev & /proc do not function yet, and need to be implemented
 */
static void create_initial_directories(void)
{
    // if (vfs_mkdir("/proc", 0) < 0) {
    //     abort("Could not create /proc");
    // }
    // if (vfs_mkdir("/var", 0) < 0) {
    //     abort("Could not create /var");
    // }
    // if (vfs_mkdir("/dev", 0) < 0) {
    //     abort("Could not create /dev");
    // }
    // if (vfs_mkdir("/sys", 0) < 0) {
    //     abort("Could not create /sys");
    // }
}

vfs_inode_t* vfs_lookup(vfs_inode_t* start, char const* path)
{
    if (!start || !path) {
        return NULL;
    }

    if (path[0] == '/' && path[1] == '\0') {
        return start;
    }

    char** components = split(path, '/');
    if (!components) {
        return NULL;
    }

    vfs_inode_t* current = start;
    vfs_inode_t* new = NULL;
    for (int i = 0; components[i]; i += 1) {
        if (components[i][0] == '\0') {
            continue;
        }

        s32 name_len = (s32)strlen(components[i]);
        if (current->i_op->lookup(current, components[i], name_len, &new) < 0) {
            goto error;
        }

        current = new;
    }

    for (int i = 0; components[i]; i += 1) {
        kfree(components[i]);
    }
    kfree(components);

    return current;

error:
    for (int i = 0; components[i]; i += 1) {
        kfree(components[i]);
    }
    kfree(components);

    return NULL;
}

vfs_superblock_t* vfs_mount_root(block_device_t* root_device)
{
    vfs_superblock_t* sb = kmalloc(sizeof(vfs_superblock_t));
    if (!sb) {
        abort("Could not allocate the root superblock");
    }
    memset(sb, 0, sizeof(vfs_superblock_t));
    sb->s_dev = root_device->d_dev;
    sb->s_op = &ext2_sops;

    vfs_superblock_t* result = NULL;
    for (int i = 0; file_systems[i].name; i += 1) {
        result = file_systems[i].read_super(sb, NULL, 0);
        if (result) {
            return result;
        }
    }

    kfree(sb);
    abort("Root Device Filesystem is not supported");
    __builtin_unreachable();
}

void vfs_init(void)
{
    block_device_t* d = get_device(0);
    vfs_superblock_t* sb = vfs_mount_root(d);
    root_inode = sb->s_root_node;

    if (!root_inode) {
        abort("Root Device is missing");
    }

    create_initial_directories();
}

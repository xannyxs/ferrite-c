#include "fs/vfs.h"
#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "fs/ext2/ext2.h"
#include "fs/vfs/mode_t.h"
#include "lib/stdlib.h"
#include "lib/string.h"
#include "memory/kmalloc.h"
#include "sys/file/stat.h"
#include <ferrite/types.h>

extern ext2_mount_t
    ext2_mounts[MAX_EXT2_MOUNTS]; // FIXME: Assuming it is always ext2 and same
                                  // mount pount

static vfs_inode_t* g_root_inode = NULL;

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
    // Check if it already exists?

    if (vfs_mkdir("/proc", 0) < 0) {
        abort("Could not create /proc");
    }
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

int vfs_lookup(char const* path)
{

    printk("path: '%s'\n", path);
    if (path) {
        char** split_path = split(path, '/');
        if (!split_path) {
            return -1;
        }
        vfs_inode_t* current = g_root_inode;
        for (int i = 0; split_path[i]; i += 1) {
            vfs_inode_t* new
                = g_root_inode->i_op->lookup(current, split_path[i]);
            if (!new) {
                return -1;
            }

            if (current != g_root_inode) {
                kfree(current);
            }
            current = new;
        }

        for (int i = 0; split_path[i]; i += 1) {
            kfree(split_path[i]);
        }
        kfree(split_path);
    } else {
        vfs_inode_t* new = g_root_inode->i_op->lookup(g_root_inode, "/");
        if (!new) {
            return -1;
        }
        kfree(new);
    }

    return 0;
}

void vfs_mount_root(void)
{
    g_root_inode = ext2_get_root_node();
    if (!g_root_inode) {
        abort("No root inode!");
    }
}

s32 vfs_mkdir(char const* path, mode_t mode)
{
    if (path[0] != '/') {
        return -1;
    }

    if (!g_root_inode) {
        return -1;
    }

    char** split_path = split(path, '/');
    if (!split_path) {
        return -1;
    }

    char* str = split_path[0];
    vfs_inode_t* current = g_root_inode;
    for (int i = 0; split_path[i + 1]; i += 1) {
        vfs_inode_t* new = current->i_op->lookup(current, str);
        if (!new) {
            goto error;
        }

        str = split_path[i];

        if (current != g_root_inode) {
            kfree(current);
        }
        current = new;
    }

    vfs_dentry_t entry;
    entry.d_name = str;
    entry.d_inode = NULL;

    mode |= S_IFDIR;
    if (current->i_op->mkdir(current, &entry, mode) < 0) {
        goto error;
    }

    kfree(current);
    for (int i = 0; split_path[i]; i += 1) {
        kfree(split_path[i]);
    }

    if (entry.d_inode) {
        // TODO: Need to create dcache to not constantly call certain IO
        // functions
        kfree(entry.d_inode);
    }
    kfree(split_path);
    return 0;

error:
    if (current != g_root_inode) {
        kfree(current);
    }

    for (int i = 0; split_path[i]; i += 1) {
        kfree(split_path[i]);
    }

    kfree(split_path);
    return -1;
}

void vfs_init(void)
{
    vfs_mount_root();
    create_initial_directories();
}

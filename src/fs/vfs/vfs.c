#include "fs/vfs.h"
#include "fs/ext2/ext2.h"
#include "lib/stdlib.h"

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

void vfs_mount_root(void)
{
    g_root_inode = ext2_get_root_node();
    if (!g_root_inode) {
        abort("No root inode!");
    }
}

void vfs_init(void)
{
    vfs_mount_root();
    create_initial_directories();
}

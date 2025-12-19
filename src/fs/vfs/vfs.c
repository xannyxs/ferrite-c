#include "fs/vfs.h"
#include "drivers/block/device.h"
#include "fs/filesystem.h"
#include "lib/stdlib.h"
#include "memory/kmalloc.h"

#include <ferrite/string.h>
#include <ferrite/types.h>

extern struct super_operations ext2_sops;
vfs_inode_t* root_inode = NULL;

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
}

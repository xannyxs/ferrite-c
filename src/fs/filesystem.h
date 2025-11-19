#ifndef FS_FILESYSTEM_H
#define FS_FILESYSTEM_H

#include "fs/ext2/ext2.h"
#include "fs/vfs.h"

struct file_system_type {
    vfs_superblock_t* (*read_super)(vfs_superblock_t*, void*, int);
    char* name;
    int requires_dev;
};

struct file_system_type file_systems[]
    = { { ext2_superblock_read, "ext2", 1 }, { NULL, NULL, 0 } };

#endif

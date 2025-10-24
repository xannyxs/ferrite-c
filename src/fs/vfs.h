#ifndef FS_VFS_H
#define FS_VFS_H

#include "fs/ext2/ext2.h"
#include "types.h"

typedef struct {
    u32 i_ino;
    u32 i_mode;
    u32 size;

    void* fs_specific;
    struct inode_operations* i_op;
} vfs_inode_t;

struct inode_operations {
    vfs_inode_t* (*lookup)(vfs_inode_t* inode, char const*);
    vfs_inode_t* (*inode_get)(u32 inode_num);
    void (*inode_put)(vfs_inode_t*);
};

#endif /* FS_VFS_H */

#ifndef FS_VFS_H
#define FS_VFS_H

#include "types.h"

typedef struct {
    u32 i_ino;
    u32 i_mode;
    u32 i_size;

    void* i_fs_specific;
    struct inode_operations* i_op;

    u8 i_name[];
} vfs_inode_t;

typedef struct {
    char* d_name;
    vfs_inode_t* d_inode;
} vfs_dentry_t;

struct inode_operations {
    vfs_inode_t* (*lookup)(vfs_inode_t* inode, char const*);
    vfs_inode_t* (*inode_get)(u32 inode_num);
    void (*inode_put)(vfs_inode_t*);

    int (*read)(vfs_inode_t* vfs, void* buff, u32 offset, u32 len);
    int (*write)(vfs_inode_t* vfs, void const* buff, u32 offset, u32 len);
    vfs_inode_t* (*create)(vfs_inode_t* vfs, char const* name, u32 mode);
};

#endif /* FS_VFS_H */

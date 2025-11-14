#ifndef FS_VFS_H
#define FS_VFS_H

#include "fs/vfs/mode_t.h"
#include "types.h"

typedef struct vfs_inode {
    u32 i_ino;
    u32 i_dev;
    u32 i_size;
    u16 i_mode;
    u16 i_count;

    struct inode_operations* i_op;

    union {
        struct socket* i_socket;
        struct ext2_inode* i_ext2;
    } u;
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
    int (*create)(vfs_inode_t* parent, vfs_dentry_t* dentry, mode_t mode);
    int (*mkdir)(vfs_inode_t* parent, vfs_dentry_t* dentry, mode_t mode);
};

vfs_inode_t* inode_get(mode_t);

void inode_put(vfs_inode_t*);

s32 vfs_mkdir(char const* path, mode_t mode);

void vfs_init(void);

#endif /* FS_VFS_H */

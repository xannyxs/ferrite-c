#ifndef FS_VFS_H
#define FS_VFS_H

#include "ferrite/dirent.h"
#include "fs/ext2/ext2.h"
#include "fs/vfs/mode_t.h"
#include "sys/file/file.h"

#include <ferrite/types.h>

typedef struct {
    dev_t s_dev;
    unsigned long s_blocksize;

    struct super_operations* s_op;
    union {
        ext2_super_t* ext2_sb;
    } u;
} vfs_superblock_t;

typedef struct vfs_inode {
    unsigned long i_ino;
    u32 i_dev;
    u32 i_size;
    u16 i_mode;
    u16 i_count;

    vfs_superblock_t* i_sb;

    struct inode_operations* i_op;
    union {
        struct ext2_inode* i_ext2;
    } u;
} vfs_inode_t;

typedef struct vfs_dentry {
    char* d_name;
    vfs_inode_t* d_inode;
} vfs_dentry_t;

struct super_operations {
    void (*read_inode)(vfs_inode_t*);
    void (*write_inode)(vfs_inode_t*);
    void (*put_inode)(vfs_inode_t*);
    void (*put_super)(vfs_superblock_t*);
    void (*write_super)(vfs_superblock_t*);
};

struct inode_operations {
    struct file_operations* default_file_ops;

    vfs_inode_t* (*lookup)(vfs_inode_t* inode, char const*);
    vfs_inode_t* (*inode_get)(u32 inode_num);
    void (*inode_put)(vfs_inode_t*);

    int (*create)(vfs_inode_t* parent, vfs_dentry_t* dentry, mode_t mode);
    int (*mkdir)(vfs_inode_t* parent, char const*, int, int);
};

vfs_inode_t* inode_get(mode_t);

void inode_put(vfs_inode_t*);

s32 vfs_mkdir(char const* path, mode_t mode);

void vfs_init(void);

#endif /* FS_VFS_H */

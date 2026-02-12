#ifndef FS_VFS_H
#define FS_VFS_H

#include "sys/file/file.h"
#include <ferrite/socket.h>

#include <types.h>

struct vfs_mount;
struct ext2_superblock;
struct ext2_block_group_descriptor;

#define MAY_EXEC 1
#define MAY_WRITE 2
#define MAY_READ 4

#define MS_RDONLY 1   /* mount read-only */
#define MS_NOSUID 2   /* ignore suid and sgid bits */
#define MS_NODEV 4    /* disallow access to device special files */
#define MS_NOEXEC 8   /* disallow program execution */
#define MS_SYNC 16    /* writes are synced at once */
#define MS_REMOUNT 32 /* alter flags of a mounted FS */

#define IS_RDONLY(inode) \
    (((inode)->i_sb) && ((inode)->i_sb->s_flags & MS_RDONLY))
#define IS_NOSUID(inode) ((inode)->i_flags & MS_NOSUID)
#define IS_NOEXEC(inode) ((inode)->i_flags & MS_NOEXEC)

typedef struct {
    dev_t s_dev;
    unsigned long s_blocksize;

    struct vfs_inode* s_root_node;

    struct super_operations* s_op;
    union {
        struct {
            struct ext2_superblock* s_es;
            struct ext2_block_group_descriptor* s_group_desc;
            u32 s_groups_count;
        } ext2_sb;
    } u;
} vfs_superblock_t;

typedef struct vfs_inode {
    dev_t i_dev; // Block device this filesystem is mounted on (e.g., /dev/hda0)
    dev_t i_rdev; // Device number this special file represents (for
                  // S_IFCHR/S_IFBLK only)

    unsigned long i_ino;

    uid_t i_uid;
    gid_t i_gid;

    u32 i_size;
    u16 i_mode;
    u16 i_count;
    u16 i_links_count;

    time_t i_atime;
    time_t i_mtime;
    time_t i_ctime;

    struct vfs_inode* i_mount;
    vfs_superblock_t* i_sb;

    struct inode_operations* i_op;
    union {
        struct socket* i_socket;
        struct ext2_inode* i_ext2;
    } u;
} vfs_inode_t;

typedef struct vfs_dentry {
    char* d_name;
    vfs_inode_t* d_inode;
} vfs_dentry_t;

struct super_operations {
    s32 (*read_inode)(vfs_inode_t*);

    s32 (*write_inode)(vfs_inode_t*);
    s32 (*write_super)(vfs_superblock_t*);

    void (*put_inode)(vfs_inode_t*);
    void (*put_super)(vfs_superblock_t*);
};

struct inode_operations {
    struct file_operations* default_file_ops;

    int (*lookup)(vfs_inode_t*, char const*, int, vfs_inode_t**);
    vfs_inode_t* (*inode_get)(u32);
    void (*inode_put)(vfs_inode_t*);

    int (*create)(vfs_inode_t*, char const*, int, int, vfs_inode_t**);
    int (*mkdir)(vfs_inode_t*, char const*, int, int);
    int (*rmdir)(vfs_inode_t*, char const*, int);
    int (*mknod)(vfs_inode_t*, char const*, int, int, int);

    int (*unlink)(vfs_inode_t*, char const*, int);
    int (*truncate)(vfs_inode_t*, off_t);
    int (*permission)(vfs_inode_t*, int);
};

vfs_inode_t* inode_get_empty(vfs_superblock_t* sb, unsigned long ino);

vfs_inode_t* inode_get(vfs_superblock_t* sb, unsigned long ino);

void inode_put(vfs_inode_t*);

void vfs_init(void);

/* namei.c */

int in_group_p(gid_t grp);

int vfs_permission(vfs_inode_t* node, int mask);

vfs_inode_t* vfs_lookup(char const*);

int vfs_mknod(char const*, int, dev_t);

/* devfs */

void devfs_init(void);

#endif /* FS_VFS_H */

#ifndef FILE_H
#define FILE_H

#include "ferrite/dirent.h"
#include "fs/vfs/mode_t.h"

#include <ferrite/types.h>

struct vfs_inode;

#define FMODE_READ 0x01
#define FMODE_WRITE 0x02

typedef u16 dev_t;
typedef long off_t;

typedef struct file {
    struct vfs_inode* f_inode;
    struct file_operations* f_op;

    off_t f_pos;
    mode_t f_mode;
    u16 f_flags;
    u16 f_count;
} file_t;

struct file_operations {
    s32 (*readdir)(struct vfs_inode*, struct file*, dirent_t*, s32);

    int (*read)(struct vfs_inode*, struct file*, void*, int);
    int (*write)(struct vfs_inode*, struct file*, void const*, int);

    int (*open)(struct vfs_inode*, struct file*);
    void (*release)(struct vfs_inode*, struct file*);
    int (*lseek)(struct vfs_inode*, struct file*, off_t, int);
};

int fd_alloc(void);

struct file* file_get(void);

int fd_install(struct vfs_inode* node, int fd);

void file_put(file_t* f);

file_t* fd_get(s32 fd);

#endif

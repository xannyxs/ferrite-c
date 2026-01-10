#ifndef _FS_MOUNT_H
#define _FS_MOUNT_H

#include "fs/vfs.h"

#include <ferrite/limits.h>

#define find_mount(x)                                                                                             \
    _Generic((x), char*: find_mount_by_path, const char*: find_mount_by_path, vfs_inode_t*: find_mount_by_inode)( \
        x                                                                                                         \
    )

typedef struct vfs_mount {
    char m_name[NAME_MAX];
    vfs_inode_t* m_mount_point_inode;
    vfs_superblock_t* m_sb;
    vfs_inode_t* m_root_inode;

    struct vfs_mount* next;
} vfs_mount_t;

vfs_mount_t* find_mount_by_inode(vfs_inode_t* mounted_root);

vfs_mount_t* find_mount_by_path(char const* path);

void list_mounts(void);

int vfs_mount(char const*, char const*, char const*, unsigned long);

int vfs_unmount(char const*);

void mount_root_device(char*);

#endif

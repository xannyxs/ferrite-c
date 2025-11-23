#include "fs/vfs.h"
#include "memory/kmalloc.h"

#include <ferrite/limits.h>
#include <ferrite/types.h>
#include <lib/stdlib.h>

vfs_inode_t inode_cache[MAX_INODES];

void inode_cache_init(void)
{
    for (int i = 0; i < MAX_INODES; i++) {
        inode_cache[i].i_count = 0;
    }
}

vfs_inode_t* inode_get(vfs_superblock_t* sb, unsigned long ino)
{
    if (!sb) {
        abort("inode_get: sb == NULL");
    }

    for (int i = 0; i < MAX_INODES; i += 1) {
        if (inode_cache[i].i_sb == sb && inode_cache[i].i_ino == ino) {
            return &inode_cache[i];
        }
    }

    for (int i = 0; i < MAX_INODES; i += 1) {
        if (inode_cache[i].i_count == 0) {
            inode_cache[i].i_sb = sb;
            inode_cache[i].i_ino = ino;
            inode_cache[i].i_count = 1;

            sb->s_op->read_inode(&inode_cache[i]);

            return &inode_cache[i];
        }
    }

    return NULL;
}

void inode_put(vfs_inode_t* n)
{
    if (!n) {
        return;
    }

    n->i_count -= 1;
    if (n->i_count == 0) {
        kfree(n);
    }
}

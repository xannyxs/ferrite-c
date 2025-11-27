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

/*
 * NOTE: Only minimal fields are set here:
 *   - i_sb, i_ino, i_count
 *
 * All other inode fields are loaded from disk by read_inode():
 *   - i_mode, i_uid, i_gid, i_size, i_blocks
 *   - i_atime, i_mtime, i_ctime, i_nlink
 *   - i_block[] (block pointers)
 *   - ext2-specific fields in u.i_ext2
 *
 * This is different from ext2_new_inode() which initializes
 * all fields because it's creating a NEW inode that doesn't
 * exist on disk yet.
 */
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

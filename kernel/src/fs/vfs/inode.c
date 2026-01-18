#include "fs/stat.h"
#include "fs/vfs.h"

#include <drivers/printk.h>
#include <ferrite/limits.h>
#include <ferrite/types.h>
#include <lib/stdlib.h>

vfs_inode_t inode_cache[MAX_INODES] = { 0 };

vfs_inode_t* inode_get_empty(vfs_superblock_t* sb, unsigned long ino)
{
    for (int i = 0; i < MAX_INODES; i += 1) {
        if (inode_cache[i].i_count == 0) {
            inode_cache[i].i_sb = sb;
            inode_cache[i].i_ino = ino;
            inode_cache[i].i_count = 1;

            if (sb && sb->s_op && !S_ISSOCK(inode_cache[i].i_mode)) {
                sb->s_op->read_inode(&inode_cache[i]);
            }

            return &inode_cache[i];
        }
    }

    return NULL;
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

            inode_cache[i].i_count += 1;

            // This is extremely fragile. Should take a closer look at this
            if (sb && sb->s_op && !S_ISSOCK(inode_cache[i].i_mode)) {
                sb->s_op->read_inode(&inode_cache[i]);
            }

            return &inode_cache[i];
        }
    }

    return inode_get_empty(sb, ino);
}

void inode_put(vfs_inode_t* n)
{
    if (!n) {
        return;
    }

    if (n->i_count == 0) {
        printk("warning: inode_put: already at count 0!");
        return;
    }

    n->i_count -= 1;
    if (n->i_count == 0) {
        if (n->i_sb->s_root_node == n) {
            abort("inode_put: root reached count 0!");
        }

        n->i_sb = NULL;
        n->i_ino = 0;
    }
}

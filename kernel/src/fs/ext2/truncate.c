#include "arch/x86/time/time.h"
#include "fs/ext2/ext2.h"
#include <uapi/stat.h>
#include "fs/vfs.h"
#include "lib/math.h"

#include <uapi/errno.h>
#include <types.h>

int ext2_truncate(vfs_inode_t* node, off_t len)
{
    if (!node || !S_ISREG(node->i_mode)) {
        return -EINVAL;
    }

    ext2_inode_t* ext2_node = node->u.i_ext2;
    vfs_superblock_t* sb = node->i_sb;

    if (len > (long)node->i_size) {
        time_t now = getepoch();
        node->i_size = len;
        node->i_mtime = now;
        node->i_ctime = now;

        ext2_node->i_size = node->i_size;
        ext2_node->i_mtime = node->i_mtime;
        ext2_node->i_ctime = node->i_ctime;

        return node->i_sb->s_op->write_inode(node);
    }

    u32 blocks_needed = CEIL_DIV(len, sb->s_blocksize);

    for (u32 i = blocks_needed; i < 12 && ext2_node->i_block[i]; i += 1) {
        ext2_free_block(node, ext2_node->i_block[i]);
        ext2_node->i_block[i] = 0;
    }

    time_t now = getepoch();
    node->i_size = len;
    node->i_mtime = now;
    node->i_ctime = now;

    ext2_node->i_size = node->i_size;
    ext2_node->i_mtime = node->i_mtime;
    ext2_node->i_ctime = node->i_ctime;

    u32 allocated = 0;
    for (u32 i = 0; i < 12 && ext2_node->i_block[i]; i += 1) {
        allocated += 1;
    }

    ext2_node->i_blocks = (allocated * sb->s_blocksize) / 512;

    return node->i_sb->s_op->write_inode(node);
}

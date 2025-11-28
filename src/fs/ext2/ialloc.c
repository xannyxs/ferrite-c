#include "arch/x86/bitops.h"
#include "arch/x86/time/time.h"
#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "fs/ext2/ext2.h"
#include "fs/vfs.h"
#include "lib/math.h"
#include "memory/kmalloc.h"
#include "sys/file/stat.h"

#include <ferrite/errno.h>

vfs_inode_t* ext2_new_inode(vfs_inode_t const* dir, int mode, int* err)
{
    vfs_superblock_t* sb = dir->i_sb;
    block_device_t* d = get_device(sb->s_dev);
    ext2_super_t* es = sb->u.ext2_sb.s_es;
    ext2_block_group_descriptor_t* bgd = NULL;
    u32 bgd_index;

    u32 bgd_count = CEIL_DIV(es->s_inodes_count, es->s_inodes_per_group);
    for (bgd_index = 0; bgd_index < bgd_count; bgd_index += 1) {
        if (sb->u.ext2_sb.s_group_desc[bgd_index].bg_free_inodes_count != 0) {
            bgd = &sb->u.ext2_sb.s_group_desc[bgd_index];
            break;
        }
    }

    if (!bgd) {
        if (err) {
            *err = -ENOSPC;
        }

        return NULL;
    }

    u8 bitmap[sb->s_blocksize];
    if (ext2_read_block((vfs_inode_t*)dir, bitmap, bgd->bg_inode_bitmap) < 0) {
        if (err) {
            *err = -EIO;
        }
        return NULL;
    }

    int bit = find_free_bit_in_bitmap(bitmap, sb->s_blocksize);
    if (bit < 0) {
        printk("%s: No free inode\n", __func__);
        if (err) {
            *err = -ENOSPC;
        }
        return NULL;
    }

    int node_num = (s32)(bit + (es->s_inodes_per_group * bgd_index) + 1);

    int oldbit = atomic_set_bit((s32)bit, (void*)bitmap);
    if (oldbit) {
        printk("%s: Warning: inode %u already allocated\n", __func__, node_num);
        return NULL;
    }

    bgd->bg_free_inodes_count -= 1;
    es->s_free_inodes_count -= 1;
    if (S_ISDIR(mode) == 1) {
        bgd->bg_used_dirs_count += 1;
    }

    u32 const sectors_per_block = sb->s_blocksize / d->d_sector_size;
    u32 const sector_num = bgd->bg_inode_bitmap * sectors_per_block;
    if (d->d_op->write(
            d, sector_num, sectors_per_block, bitmap, sb->s_blocksize
        )
        < 0) {

        if (err) {
            *err = -EIO;
        }
        return NULL;
    }

    if (ext2_bgd_write(sb, bgd_index) < 0) {
        if (err) {
            *err = -EIO;
        }
        return NULL;
    }

    if (sb->s_op->write_super(sb) < 0) {
        if (err) {
            *err = -EIO;
        }
        return NULL;
    }

    vfs_inode_t* inode = inode_get(sb, node_num);
    if (!inode) {
        if (err) {
            *err = -ENOMEM;
        }
        return NULL;
    }

    ext2_inode_t* ext2_inode = kmalloc(sizeof(ext2_inode_t));
    if (!ext2_inode) {
        if (err) {
            *err = -ENOMEM;
        }

        inode_put(inode);
        return NULL;
    }

    time_t now = getepoch();

    inode->i_ino = node_num;
    inode->i_sb = sb;
    inode->i_mode = mode;
    inode->i_uid = 0;
    inode->i_gid = 0;
    inode->i_size = 0;
    inode->i_atime = now;
    inode->i_mtime = now;
    inode->i_ctime = now;
    inode->i_links_count = 1;

    ext2_inode->i_mode = mode;
    ext2_inode->i_blocks = 0;
    ext2_inode->i_flags = 0;
    ext2_inode->i_generation = 0;
    ext2_inode->i_file_acl = 0;
    ext2_inode->i_dir_acl = 0;
    ext2_inode->i_faddr = 0;

    for (int i = 0; i < 15; i++) {
        ext2_inode->i_block[i] = 0;
    }

    if (err) {
        *err = 0;
    }

    return inode;
}

int mark_inode_free(vfs_inode_t* node)
{
    (void)node;
    return -1;
}

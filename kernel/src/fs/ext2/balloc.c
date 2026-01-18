#include "arch/x86/bitops.h"
#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "fs/ext2/ext2.h"
#include "fs/vfs.h"
#include "lib/math.h"
#include "memory/kmalloc.h"

#include <ferrite/errno.h>
#include <ferrite/string.h>

int ext2_new_block(vfs_inode_t const* node, int* err)
{
    vfs_superblock_t* sb = node->i_sb;
    block_device_t* d = get_device(sb->s_dev);
    ext2_super_t* es = sb->u.ext2_sb.s_es;
    ext2_block_group_descriptor_t* bgd = NULL;
    u32 bgd_index;

    if (es->s_free_blocks_count == 0) {
        if (err) {
            *err = -ENOSPC;
        }
        return 0;
    }

    u8* bitmap = kmalloc(sb->s_blocksize);
    if (!bitmap) {
        return -ENOMEM;
    }

    u8* zero_block = kmalloc(sb->s_blocksize);
    if (!zero_block) {
        kfree(bitmap);
        return -ENOMEM;
    }

retry:
    bgd = NULL;
    u32 bgd_count = CEIL_DIV(es->s_blocks_count, es->s_blocks_per_group);
    for (bgd_index = 0; bgd_index < bgd_count; bgd_index += 1) {
        if (sb->u.ext2_sb.s_group_desc[bgd_index].bg_free_blocks_count != 0) {
            bgd = &node->i_sb->u.ext2_sb.s_group_desc[bgd_index];
            break;
        }
    }

    if (!bgd) {
        printk("%s: No free block\n", __func__);
        if (err) {
            *err = -ENOSPC;
        }

        kfree(bitmap);
        kfree(zero_block);
        return -1;
    }

    if (ext2_read_block(node, bitmap, bgd->bg_block_bitmap) < 0) {
        if (err) {
            *err = -EIO;
        }

        kfree(bitmap);
        kfree(zero_block);
        return -1;
    }

    int bit = find_free_bit_in_bitmap(bitmap, node->i_sb->s_blocksize);
    if (bit < 0) {
        if (err) {
            *err = -ENOSPC;
        }

        kfree(bitmap);
        kfree(zero_block);
        return -1;
    }

    int oldbit = atomic_set_bit((s32)bit, (void*)bitmap);
    if (oldbit) {
        printk("%s: Race condition on bit %d, retrying\n", __func__, bit);

        goto retry;
    }

    u32 block_num
        = bit + (es->s_blocks_per_group * bgd_index) + es->s_first_data_block;

    bgd->bg_free_blocks_count -= 1;
    es->s_free_blocks_count -= 1;

    u32 const count = sb->s_blocksize / d->d_sector_size;
    u32 const sector_num = bgd->bg_block_bitmap * count;
    if (d->d_op->write(d, sector_num, count, bitmap, sb->s_blocksize) < 0) {
        kfree(bitmap);
        kfree(zero_block);
        return -1;
    }

    if (ext2_bgd_write(sb, bgd_index) < 0) {
        if (err) {
            *err = -EIO;
        }

        kfree(bitmap);
        kfree(zero_block);

        return -1;
    }

    if (sb->s_op->write_super(sb) < 0) {
        if (err) {
            *err = -EIO;
        }

        kfree(bitmap);
        kfree(zero_block);
        return -1;
    }

    memset(zero_block, 0, sb->s_blocksize);
    u32 block_sector = block_num * count;
    if (d->d_op->write(d, block_sector, count, zero_block, sb->s_blocksize)
        < 0) {
        printk("%s: Warning: failed to zero block %u\n", __func__, block_num);
    }

    if (err) {
        *err = 0;
    }

    kfree(bitmap);
    kfree(zero_block);
    return (int)block_num;
}

int ext2_free_block(vfs_inode_t* node, u32 block_num)
{
    vfs_superblock_t* sb = node->i_sb;
    block_device_t* d = get_device(sb->s_dev);
    ext2_super_t* es = sb->u.ext2_sb.s_es;
    u32 bgd_index
        = (block_num - es->s_first_data_block) / es->s_blocks_per_group;
    ext2_block_group_descriptor_t* bgd = &sb->u.ext2_sb.s_group_desc[bgd_index];

    u8* bitmap = kmalloc(sb->s_blocksize);
    if (!bitmap) {
        return -ENOMEM;
    }

    if (ext2_read_block(node, bitmap, bgd->bg_block_bitmap) < 0) {
        kfree(bitmap);
        return -EIO;
    }

    int bit = (block_num - es->s_first_data_block) % es->s_blocks_per_group;
    int oldbit = atomic_clear_bit(bit, (void*)bitmap);
    if (!oldbit) {
        printk("%s: Warning: block %d already free\n", __func__, block_num);
        kfree(bitmap);
        return -EINVAL;
    }

    bgd->bg_free_blocks_count += 1;
    es->s_free_blocks_count += 1;

    u32 const count = sb->s_blocksize / d->d_sector_size;
    u32 const sector_num = bgd->bg_block_bitmap * count;
    if (d->d_op->write(d, sector_num, count, bitmap, sb->s_blocksize) < 0) {
        kfree(bitmap);
        return -EIO;
    }

    kfree(bitmap);
    if (ext2_bgd_write(sb, bgd_index) < 0) {
        return -EIO;
    }

    if (sb->s_op->write_super(sb) < 0) {
        return -EIO;
    }

    return 0;
}

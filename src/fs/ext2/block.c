#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "fs/ext2/ext2.h"
#include "fs/vfs.h"
#include "lib/math.h"

#include <lib/string.h>
#include <stdbool.h>

int find_free_block(vfs_inode_t* node)
{
    ext2_super_t* es = node->i_sb->u.ext2_sb.s_es;
    ext2_block_group_descriptor_t* bgd = NULL;

    u32 i;
    u32 bgd_count = CEIL_DIV(es->s_blocks_count, es->s_blocks_per_group);
    for (i = 0; i < bgd_count; i += 1) {
        if (node->i_sb->u.ext2_sb.s_group_desc[i].bg_free_blocks_count != 0) {
            bgd = &node->i_sb->u.ext2_sb.s_group_desc[i];
            break;
        }
    }

    if (!bgd) {
        printk("%s: No free block\n", __func__);
        return -1;
    }

    u8 bitmap[node->i_sb->s_blocksize];
    if (ext2_read_block(node, bitmap, bgd->bg_block_bitmap) < 0) {
        return -1;
    }

    int bit = find_free_bit_in_bitmap(bitmap, node->i_sb->s_blocksize);
    if (bit < 0) {
        return -1;
    }

    return (s32)(bit + (es->s_blocks_per_group * i));
}

int mark_block_bitmap(vfs_inode_t* node, u32 block_num, bool allocate)
{
    vfs_superblock_t* sb = node->i_sb;
    block_device_t* d = get_device(sb->s_dev);
    if (!d) {
        return -1;
    }

    ext2_super_t* es = sb->u.ext2_sb.s_es;

    u32 bgd_index = block_num / es->s_blocks_per_group;
    ext2_block_group_descriptor_t* bgd
        = &node->i_sb->u.ext2_sb.s_group_desc[bgd_index];

    u8 bitmap[sb->s_blocksize];
    if (ext2_read_block(node, bitmap, bgd->bg_block_bitmap) < 0) {
        return -1;
    }

    u32 local_block_index = block_num % es->s_blocks_per_group;
    u32 byte_index = local_block_index / 8;
    u32 bit_index = local_block_index % 8;

    if (allocate) {
        bitmap[byte_index] |= (1 << bit_index);
        bgd->bg_free_blocks_count--;
        es->s_free_blocks_count--;
    } else {
        bitmap[byte_index] &= ~(1 << bit_index);
        bgd->bg_free_blocks_count++;
        es->s_free_blocks_count++;
    }

    u32 count = sb->s_blocksize / d->d_sector_size;
    u32 sector_num = bgd->bg_block_bitmap * count;
    if (d->d_op->write(d, sector_num, count, bitmap, sb->s_blocksize) < 0) {
        return -1;
    }

    // return ext2_block_group_descriptors_write(m, bgd_index);
    return 0;
}

inline int mark_block_allocated(vfs_inode_t* node, u32 block_num)
{
    return mark_block_bitmap(node, block_num, true);
}

inline int mark_block_free(vfs_inode_t* node, u32 block_num)
{
    return mark_block_bitmap(node, block_num, false);
}

s32 ext2_read_block(vfs_inode_t* node, u8* buff, u32 block_num)
{
    block_device_t* d = get_device(node->i_sb->s_dev);

    if (!d) {
        printk("%s: device is NULL\n", __func__);
        return -1;
    }

    if (!d->d_op || !d->d_op->read) {
        printk("%s: device has no read operation\n", __func__);
        return -1;
    }

    vfs_superblock_t* sb = node->i_sb;
    u32 count = sb->s_blocksize / d->d_sector_size;
    u32 sector_pos = block_num * count;

    if (d->d_op->read(d, sector_pos, count, buff, sb->s_blocksize) < 0) {
        return -1;
    }

    return 0;
}

s32 ext2_write_block(
    vfs_inode_t* node,
    u32 block_num,
    void const* buff,
    u32 offset,
    u32 len
)
{
    vfs_superblock_t* sb = node->i_sb;
    block_device_t* d = get_device(sb->s_dev);
    if (!d) {
        printk("%s: device is null\n", __func__);
        return -1;
    }

    if (!d->d_op || !d->d_op->write || !d->d_op->read) {
        printk("%s: device has no write or read operation\n", __func__);
        return -1;
    }

    u32 start_byte = block_num * sb->s_blocksize;
    u32 sector_pos = start_byte / d->d_sector_size;
    u32 count = sb->s_blocksize / d->d_sector_size;

    u8 tmp[sb->s_blocksize];
    if (d->d_op->read(d, sector_pos, count, tmp, sb->s_blocksize) < 0) {
        return -1;
    }

    memcpy(&tmp[offset], buff, len);

    if (d->d_op->write(d, sector_pos, count, tmp, sb->s_blocksize) < 0) {
        return -1;
    }

    return 0;
}

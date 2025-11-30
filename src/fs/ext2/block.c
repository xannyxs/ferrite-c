#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "fs/ext2/ext2.h"
#include "fs/vfs.h"

#include <lib/string.h>
#include <stdbool.h>

s32 ext2_read_block(vfs_inode_t const* node, u8* buff, u32 block_num)
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

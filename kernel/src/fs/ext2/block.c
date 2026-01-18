#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "fs/ext2/ext2.h"
#include "fs/vfs.h"
#include "memory/kmalloc.h"

#include <ferrite/errno.h>
#include <ferrite/string.h>
#include <stdbool.h>

s32 ext2_read_block(vfs_inode_t const* node, u8* buff, u32 block_num)
{
    block_device_t* d = get_device(node->i_sb->s_dev);
    if (!d || !d->d_op || !d->d_op->read) {
        printk("%s: device has no read operation\n", __func__);
        return -EIO;
    }

    vfs_superblock_t* sb = node->i_sb;
    u32 count = sb->s_blocksize / d->d_sector_size;
    u32 sector_pos = block_num * count;

    return d->d_op->read(d, sector_pos, count, buff, sb->s_blocksize);
}

int ext2_write_block(
    vfs_inode_t* node,
    u32 block_num,
    void const* buff,
    u32 offset,
    u32 len
)
{
    int retval = 0;
    vfs_superblock_t* sb = node->i_sb;
    block_device_t* d = get_device(sb->s_dev);
    if (!d || !d->d_op || !d->d_op->write || !d->d_op->read) {
        printk("%s: device has no write or read operation\n", __func__);
        return -EIO;
    }

    u32 start_byte = block_num * sb->s_blocksize;
    u32 sector_pos = start_byte / d->d_sector_size;
    u32 count = sb->s_blocksize / d->d_sector_size;

    u8* tmp = kmalloc(sb->s_blocksize);
    if (!tmp) {
        return -ENOMEM;
    }

    if (offset != 0 || len < sb->s_blocksize) {
        retval = d->d_op->read(d, sector_pos, count, tmp, sb->s_blocksize);
        if (retval < 0) {
            kfree(tmp);
            return retval;
        }
    }

    memcpy(&tmp[offset], buff, len);
    retval = d->d_op->write(d, sector_pos, count, tmp, sb->s_blocksize);

    kfree(tmp);
    return retval;
}

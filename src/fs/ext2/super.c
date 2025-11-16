#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "fs/ext2/ext2.h"
#include "lib/math.h"
#include <ferrite/types.h>

/**
 * Write the superblock to disk at offset 1024.
 *
 * @param m Mount point containing the superblock to write
 * @return 0 on success, -1 on failure
 */
s32 ext2_superblock_write(ext2_mount_t* m)
{
    block_device_t* d = m->m_device;
    if (!d) {
        printk("%s: device is NULL\n", __func__);
        return -1;
    }

    if (!d->d_op || !d->d_op->write) {
        printk("%s: device has no write operation\n", __func__);
        return -1;
    }

    u32 count = CEIL_DIV(sizeof(ext2_super_t), d->sector_size);
    u32 start_pos = 1024 / d->sector_size;

    if (d->d_op->write(
            d, start_pos, count, &m->m_superblock, sizeof(ext2_super_t)
        )
        < 0) {
        return -1;
    }

    return 0;
}

/**
 * Read the superblock from disk at offset 1024 and validate it.
 *
 * @param m Mount point to populate with superblock data
 * @return 0 on success, -1 on failure (device error or invalid magic)
 */
s32 ext2_superblock_read(ext2_mount_t* m)
{
    block_device_t* d = m->m_device;
    if (!d) {
        printk("%s: device is NULL\n", __func__);
        return -1;
    }

    if (!d->d_op || !d->d_op->read) {
        printk("%s: device has no read operation\n", __func__);
        return -1;
    }

    ext2_super_t* super = &m->m_superblock;
    u32 count = CEIL_DIV(sizeof(ext2_super_t), d->sector_size);
    u32 start_pos = 1024 / d->sector_size;

    if (d->d_op->read(d, start_pos, count, super, sizeof(ext2_super_t)) < 0) {
        return -1;
    }

    if (super->s_magic != EXT2_MAGIC) {
        printk(
            "%s: invalid magic (expected 0x%x, got 0x%x)\n", __func__,
            EXT2_MAGIC, super->s_magic
        );
        return -1;
    }

    m->m_block_size = (1024 << super->s_log_block_size);

    return 0;
}

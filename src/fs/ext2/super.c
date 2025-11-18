#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "fs/ext2/ext2.h"
#include "fs/vfs.h"
#include "lib/math.h"
#include "memory/kmalloc.h"

#include <ferrite/types.h>

extern struct super_operations ext2_sops;

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

    u32 count = CEIL_DIV(sizeof(ext2_super_t), d->d_sector_size);
    u32 start_pos = 1024 / d->d_sector_size;

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
 */
vfs_superblock_t*
ext2_superblock_read(vfs_superblock_t* sb, void* data, int silent)
{
    (void)data;
    (void)silent;

    block_device_t* d = get_device(sb->s_dev);
    if (!d) {
        printk("%s: device is NULL\n", __func__);
        return NULL;
    }

    if (!d->d_op || !d->d_op->read) {
        printk("%s: device has no read operation\n", __func__);
        return NULL;
    }

    ext2_super_t* es = kmalloc(sizeof(ext2_super_t));
    if (!es) {
        return NULL;
    }

    u32 count = CEIL_DIV(sizeof(ext2_super_t), d->d_sector_size);
    u32 start_pos = 1024 / d->d_sector_size;

    if (d->d_op->read(d, start_pos, count, es, sizeof(ext2_super_t)) < 0) {
        kfree(es);
        return NULL;
    }

    if (es->s_magic != EXT2_MAGIC) {
        printk(
            "%s: invalid magic (expected 0x%x, got 0x%x)\n", __func__,
            EXT2_MAGIC, es->s_magic
        );
        kfree(es);

        return NULL;
    }

    sb->s_blocksize = (1024 << es->s_log_block_size);
    sb->u.ext2_sb.s_es = es;
    sb->s_op = &ext2_sops;

    return sb;
}

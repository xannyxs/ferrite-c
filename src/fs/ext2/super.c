#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "fs/ext2/ext2.h"
#include "types.h"

s32 ext2_read_superblock(block_device_t* d, ext2_super_t* super)
{
    if (!d) {
        printk("%s: device is NULL\n", __func__);
        return -1;
    }

    if (!d->d_op || !d->d_op->read) {
        printk("%s: device has no read operation\n", __func__);
        return -1;
    }

    u32 size = sizeof(ext2_super_t) / d->sector_size;
    u32 block_pos = 1024 / d->sector_size;

    if (d->d_op->read(d, block_pos, size, super, sizeof(ext2_super_t)) < 0) {
        printk("%s: failed to read from device (LBA %u, "
               "count %u)\n",
            __func__, block_pos, size);
        return -1;
    }

    if (super->s_magic != EXT2_MAGIC) {
        printk("%s: invalid magic (expected 0x%x, got 0x%x)\n", __func__,
            EXT2_MAGIC, super->s_magic);
        return -1;
    }

    return 0;
}

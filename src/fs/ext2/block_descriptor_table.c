#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "fs/ext2/ext2.h"
#include "lib/math.h"
#include "types.h"

s32 ext2_read_bgd_table(
    ext2_mount_t* m, ext2_block_group_descriptor_t* bgd, u32 num_block_groups)
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

    u32 amount_of_bytes
        = num_block_groups * sizeof(ext2_block_group_descriptor_t);
    u32 size = CEIL_DIV(amount_of_bytes, d->sector_size);
    u32 bgd_pos = CEIL_DIV(1024 + sizeof(ext2_super_t), d->sector_size);

    if (d->d_op->read(d, bgd_pos, size, bgd, size * d->sector_size) < 0) {
        printk("%s: failed to read from device (LBA %u, "
               "count %u)\n",
            __func__, bgd_pos, size);
        return -1;
    }

    return 0;
}

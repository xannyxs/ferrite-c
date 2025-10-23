#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "fs/ext2/ext2.h"
#include "lib/stdlib.h"
#include "lib/string.h"
#include "types.h"

s32 ext2_read_inode(u32 index, ext2_mount_t* m, block_device_t* d)
{
    if (!d) {
        printk("%s: device is NULL\n", __func__);
        return -1;
    }

    if (!d->d_op || !d->d_op->read) {
        printk("%s: device has no read operation\n", __func__);
        return -1;
    }

    ext2_super_t* s = &m->m_superblock;
    u32 block_group = (index - 1) / s->s_inodes_per_group;
    ext2_block_group_descriptor_t* bgd = &m->m_block_group[block_group];

    u32 block_size = (1024 << s->s_log_block_size);
    u32 offset = (index - 1) * s->s_inode_size;
    u32 addr = (bgd->bg_inode_table * block_size) + offset;
    u32 sector_pos = addr / d->sector_size;

    u8 buff[d->sector_size];
    if (d->d_op->read(d, sector_pos, 1, buff, d->sector_size) < 0) {
        printk("%s: failed to read from device (LBA %u, "
               "count %u)\n",
            __func__, sector_pos, 1);
        return -1;
    }

    offset = addr % d->sector_size;
    if (offset > d->sector_size) {
        printk("Offset it bigger than sector size\n");
        return -1;
    }
    memcpy(&m->m_inode, &buff[offset], sizeof(ext2_inode_t));

    return 0;
}

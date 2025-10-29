#include "drivers/printk.h"
#include "fs/ext2/ext2.h"
#include "lib/string.h"

#include <stdbool.h>

int mark_block_bitmap(ext2_mount_t* m, u32 block_num, bool allocate)
{
    ext2_super_t s = m->m_superblock;
    u32 bgd_index = block_num / s.s_blocks_per_group;
    ext2_block_group_descriptor_t* bgd = &m->m_block_group[bgd_index];

    u8 bitmap[m->m_block_size];
    if (read_block(m, bitmap, bgd->bg_block_bitmap) < 0) {
        return -1;
    }

    u32 local_block_index = block_num % s.s_blocks_per_group;
    u32 byte_index = local_block_index / 8;
    u32 bit_index = local_block_index % 8;

    if (allocate) {
        bitmap[byte_index] |= (1 << bit_index);
        bgd->bg_free_blocks_count--;
    } else {
        bitmap[byte_index] &= ~(1 << bit_index);
        bgd->bg_free_blocks_count++;
    }

    u32 sectors_per_block = m->m_block_size / m->m_device->sector_size;
    u32 sector_num = bgd->bg_block_bitmap * sectors_per_block;
    if (m->m_device->d_op->write(
            m->m_device, sector_num, sectors_per_block, bitmap, m->m_block_size
        )
        < 0) {
        return -1;
    }

    return ext2_block_group_descriptors_write(m, bgd_index);
}

inline int mark_block_allocated(ext2_mount_t* m, u32 block_num)
{
    return mark_block_bitmap(m, block_num, true);
}

inline int mark_block_free(ext2_mount_t* m, u32 block_num)
{
    return mark_block_bitmap(m, block_num, false);
}

s32 ext2_write_block(ext2_mount_t* m, u32 block_num, void const* buff)
{
    block_device_t* d = m->m_device;
    if (!d) {
        printk("%s: device is null\n", __func__);
        return -1;
    }

    if (!d->d_op || !d->d_op->write) {
        printk("%s: device has no write operation\n", __func__);
        return -1;
    }

    u32 count = m->m_block_size / d->sector_size;
    u32 start_byte = block_num * m->m_block_size;
    u32 sector_pos = start_byte / d->sector_size;

    if (d->d_op->write(d, sector_pos, count, buff, m->m_block_size) < 0) {
        return -1;
    }

    return 0;
}

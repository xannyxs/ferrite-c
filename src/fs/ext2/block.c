#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "fs/ext2/ext2.h"
#include "lib/math.h"
#include "lib/string.h"

#include <stdbool.h>

int find_free_block(ext2_mount_t* m)
{
    ext2_super_t s = m->m_superblock;
    ext2_block_group_descriptor_t* bgd = NULL;

    u32 i;
    u32 bgd_count = CEIL_DIV(s.s_blocks_count, s.s_blocks_per_group);
    for (i = 0; i < bgd_count; i += 1) {
        if (m->m_block_group[i].bg_free_blocks_count != 0) {
            bgd = &m->m_block_group[i];
            break;
        }
    }

    if (!bgd) {
        printk("%s: No free block\n", __func__);
        return -1;
    }

    u8 bitmap[m->m_block_size];
    if (read_block(m, bitmap, bgd->bg_block_bitmap) < 0) {
        return -1;
    }

    int bit = find_free_bit_in_bitmap(bitmap, m->m_block_size);
    if (bit < 0) {
        return -1;
    }

    return (s32)(bit + (s.s_blocks_per_group * i));
}

int mark_block_bitmap(ext2_mount_t* m, u32 block_num, bool allocate)
{
    block_device_t* d = m->m_device;
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
        m->m_superblock.s_free_blocks_count--;
    } else {
        bitmap[byte_index] &= ~(1 << bit_index);
        bgd->bg_free_blocks_count++;
        m->m_superblock.s_free_blocks_count++;
    }

    u32 count = m->m_block_size / d->sector_size;
    u32 sector_num = bgd->bg_block_bitmap * count;
    if (d->d_op->write(d, sector_num, count, bitmap, m->m_block_size) < 0) {
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

s32 read_block(ext2_mount_t* m, u8* buff, u32 block_num)
{
    block_device_t* d = m->m_device;
    if (!d) {
        printk("%s: device is NULL\n", __func__);
        return -1;
    }

    if (!d->d_op || !d->d_op->read) {
        printk("%s: device has no write operation\n", __func__);
        return -1;
    }

    u32 count = m->m_block_size / d->sector_size;
    u32 sector_pos = block_num * count;

    if (d->d_op->read(d, sector_pos, count, buff, m->m_block_size) < 0) {
        return -1;
    }

    return 0;
}

s32 ext2_write_block(
    ext2_mount_t* m,
    u32 block_num,
    void const* buff,
    u32 offset,
    u32 len
)
{
    block_device_t* d = m->m_device;
    if (!d) {
        printk("%s: device is null\n", __func__);
        return -1;
    }

    if (!d->d_op || !d->d_op->write || !d->d_op->read) {
        printk("%s: device has no write or read operation\n", __func__);
        return -1;
    }

    u32 start_byte = block_num * m->m_block_size;
    u32 sector_pos = start_byte / d->sector_size;
    u32 count = m->m_block_size / d->sector_size;

    u8 tmp[m->m_block_size];
    if (d->d_op->read(d, sector_pos, count, tmp, m->m_block_size) < 0) {
        return -1;
    }

    memcpy(&tmp[offset], buff, len);

    if (d->d_op->write(d, sector_pos, count, tmp, m->m_block_size) < 0) {
        return -1;
    }

    return 0;
}

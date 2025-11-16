#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "fs/ext2/ext2.h"
#include "lib/math.h"
#include "lib/string.h"
#include <ferrite/types.h>

#include <stdbool.h>

int mark_inode_bitmap(ext2_mount_t* m, u32 inode_num, bool allocate)
{
    ext2_super_t s = m->m_superblock;
    u32 bgd_index = (inode_num - 1) / s.s_inodes_per_group;
    ext2_block_group_descriptor_t* bgd = &m->m_block_group[bgd_index];

    u8 bitmap[m->m_block_size];
    if (read_block(m, bitmap, bgd->bg_inode_bitmap) < 0) {
        return -1;
    }

    u32 local_inode_index = (inode_num - 1) % s.s_inodes_per_group;
    u32 byte_index = local_inode_index / 8;
    u32 bit_index = local_inode_index % 8;

    if (allocate) {
        if (bitmap[byte_index] & (1 << bit_index)) {
            printk(
                "%s: Warning: inode %u already allocated\n", __func__, inode_num
            );
            return -1;
        }

        bitmap[byte_index] |= (1 << bit_index);
        bgd->bg_free_inodes_count--;
        m->m_superblock.s_free_inodes_count--;
    } else {
        if (!(bitmap[byte_index] & (1 << bit_index))) {
            printk(
                "%s: Warning: inode %u already freed\n", __func__, inode_num
            );
            return -1;
        }

        bitmap[byte_index] &= ~(1 << bit_index);
        bgd->bg_free_inodes_count++;
        m->m_superblock.s_free_inodes_count++;
    }

    u32 sectors_per_block = m->m_block_size / m->m_device->sector_size;
    u32 sector_num = bgd->bg_inode_bitmap * sectors_per_block;
    if (m->m_device->d_op->write(
            m->m_device, sector_num, sectors_per_block, bitmap, m->m_block_size
        )
        < 0) {
        return -1;
    }

    if (ext2_block_group_descriptors_write(m, bgd_index) < 0) {
        return -1;
    }

    if (ext2_superblock_write(m) < 0) {
        return -1;
    }

    return 0;
}

inline int mark_inode_allocated(ext2_mount_t* m, u32 inode_num)
{
    return mark_inode_bitmap(m, inode_num, true);
}

inline int mark_inode_free(ext2_mount_t* m, u32 inode_num)
{
    return mark_inode_bitmap(m, inode_num, false);
}

int find_free_inode(ext2_mount_t* m)
{
    ext2_super_t s = m->m_superblock;
    ext2_block_group_descriptor_t* bgd = NULL;

    u32 i;
    u32 bgd_count = CEIL_DIV(s.s_inodes_count, s.s_inodes_per_group);
    for (i = 0; i < bgd_count; i += 1) {
        if (m->m_block_group[i].bg_free_inodes_count != 0) {
            bgd = &m->m_block_group[i];
            break;
        }
    }

    if (!bgd) {
        printk("%s: No free inode\n", __func__);
        return -1;
    }

    u8 bitmap[m->m_block_size];
    read_block(m, bitmap, bgd->bg_inode_bitmap);

    int bit = find_free_bit_in_bitmap(bitmap, m->m_block_size);
    if (bit < 0) {
        return -1;
    }

    return (s32)(bit + (s.s_inodes_per_group * i) + 1);
}

s32 ext2_write_inode(ext2_mount_t* m, u32 inode_num, ext2_inode_t* inode)
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

    ext2_super_t* s = &m->m_superblock;

    u32 block_group = (inode_num - 1) / s->s_inodes_per_group;
    ext2_block_group_descriptor_t* bgd = &m->m_block_group[block_group];

    u32 local_inode_index = (inode_num - 1) % s->s_inodes_per_group;
    u32 inode_table_offset = local_inode_index * s->s_inode_size;

    u32 inode_table_start_byte = bgd->bg_inode_table * m->m_block_size;
    u32 absolute_addr = inode_table_start_byte + inode_table_offset;

    u32 sector_pos = absolute_addr / d->sector_size;
    u32 offset_in_sector = absolute_addr % d->sector_size;

    u8 buff[d->sector_size];
    if (d->d_op->read(d, sector_pos, 1, buff, d->sector_size) < 0) {
        return -1;
    }

    memcpy(&buff[offset_in_sector], inode, s->s_inode_size);

    if (d->d_op->write(d, sector_pos, 1, buff, d->sector_size) < 0) {
        return -1;
    }

    return 0;
}

s32 ext2_read_inode(ext2_mount_t* m, u32 inode_num, ext2_inode_t* inode)
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

    ext2_super_t* s = &m->m_superblock;
    u32 block_group = (inode_num - 1) / s->s_inodes_per_group;
    ext2_block_group_descriptor_t* bgd = &m->m_block_group[block_group];

    u32 offset = (inode_num - 1) * s->s_inode_size;
    u32 addr = (bgd->bg_inode_table * m->m_block_size) + offset;
    u32 sector_pos = addr / d->sector_size;

    u8 buff[d->sector_size];
    if (d->d_op->read(d, sector_pos, 1, buff, d->sector_size) < 0) {
        printk(
            "%s: failed to read from device (LBA %u, "
            "count %u)\n",
            __func__, sector_pos, 1
        );
        return -1;
    }

    offset = addr % d->sector_size;
    if (offset > d->sector_size) {
        printk("Offset it bigger than sector size\n");
        return -1;
    }
    memcpy(inode, &buff[offset], sizeof(ext2_inode_t));

    return 0;
}
